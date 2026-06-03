#include "PolicyPCSignature.h"
#include <algorithm>

PolicyPCSignature::PolicyPCSignature(int size, int n, int k, int th)
    : PolicyBase(size), N(n), K(k), threshold(th) {
    physicalFrames.resize(size, ' '); 
}

void PolicyPCSignature::updateHistory(PCSignature& sig, char page) {
    if ((int)sig.history.size() < N) {
        sig.history.push_back(page);
    } else {
        sig.history[sig.head] = page;
        sig.head = (sig.head + 1) % N;
    }
}

bool PolicyPCSignature::isLoop(const PCSignature& sig) {
    std::unordered_set<char> s;
    for (char c : sig.history) {
        if (s.count(c)) return true;
        s.insert(c);
    }
    return false;
}

bool PolicyPCSignature::isStrong(const PCSignature& sig) {
    return sig.faultWeight >= threshold;
}

std::string PolicyPCSignature::getHistoryString(const PCSignature& sig) {
    if (sig.history.empty()) return "[]";
    std::string s = "[";
    for (int i = 0; i < (int)sig.history.size(); ++i) {
        s += std::string(1, sig.history[i]);
        if (i < (int)sig.history.size() - 1) s += ", ";
    }
    s += "]";
    return s;
}

std::string PolicyPCSignature::getSignatureClass(const PCSignature& sig) {
    bool loop = isLoop(sig);
    bool strong = isStrong(sig);
    if (strong && loop) return "Strong Loop";
    if (strong && !loop) return "Strong Scan";
    if (!strong && loop) return "Weak Loop";
    return "Weak Scan";
}

void PolicyPCSignature::touch(char page) {
    if (lruPos.count(page)) {
        lruList.erase(lruPos[page]);
    }
    lruList.push_front(page);
    lruPos[page] = lruList.begin();
}

void PolicyPCSignature::promoteToMRU(char page) {
    touch(page);
}

void PolicyPCSignature::insertBeforeLRU(char page) {
    if (lruPos.count(page)) lruList.erase(lruPos[page]);
    if (lruList.empty()) {
        lruList.push_back(page);
        lruPos[page] = lruList.begin();
        return;
    }
    auto it = std::prev(lruList.end());
    lruPos[page] = lruList.insert(it, page);
}

char PolicyPCSignature::selectVictim() {
    if (lruList.empty()) return ' ';
    int retry = 0;
    int limit = (int)lruList.size() / 2;
    auto it = std::prev(lruList.end());

    while (true) {
        char candidate = *it;
        bool protect = false;

        if (pageToPCs.count(candidate)) {
            for (int pc : pageToPCs[candidate]) {
                PCSignature& sig = pcMap[pc];
                if (isStrong(sig) && isLoop(sig)) {
                    protect = true;
                    break;
                }
            }
        }

        if (!protect || retry >= limit) {
            lruList.erase(it);
            lruPos.erase(candidate);
            return candidate;
        }
        retry++;
        if (it == lruList.begin()) it = std::prev(lruList.end());
        else --it;
    }
}

void PolicyPCSignature::aging() {
    for (auto it = pcMap.begin(); it != pcMap.end();) {
        PCSignature& sig = it->second;
        sig.faultWeight /= 2;

        if (tick - sig.lastAccessTick > K * 2) {
            for (char p : sig.accessedPages) {
                pageToPCs[p].erase(it->first);
            }
            it = pcMap.erase(it);
        } else {
            ++it;
        }
    }
}

void PolicyPCSignature::Operate(char page) {
    std::vector<int> pcs;
    pcs.push_back((int)page);
    OperateWithPCs(page, pcs);
}

void PolicyPCSignature::OperateWithPCs(char page, const std::vector<int>& pcs) {
    tick++;
    StepRecord record;
    record.reqChar = page;
    record.clockHand = -1; 
    
    bool hit = frames.count(page);
    int targetRow = -1;

    for (int pc : pcs) {
        PCSignature& sig = pcMap[pc];
        sig.lastAccessTick = tick;
        updateHistory(sig, page);
        sig.accessedPages.insert(page);
        if (!hit) sig.faultWeight++;
        pageToPCs[page].insert(pc);
    }

    if (hit) {
        record.status = PageStatus::Hit;
        hitCount++;
        for (int i = 0; i < frameSize; ++i) {
            if (physicalFrames[i] == page) {
                targetRow = i; break;
            }
        }
    } else {
        record.status = PageStatus::Fault;
        faultCount++;
        
        if ((int)frames.size() >= frameSize) {
            char victim = selectVictim();
            frames.erase(victim);
            pageToPCs.erase(victim);
            migrationCount++;
            for (int i = 0; i < frameSize; ++i) {
                if (physicalFrames[i] == victim) {
                    physicalFrames[i] = page;
                    targetRow = i; break;
                }
            }
        } else {
            for (int i = 0; i < frameSize; ++i) {
                if (physicalFrames[i] == ' ') {
                    physicalFrames[i] = page;
                    targetRow = i; break;
                }
            }
        }
        frames.insert(page);
    }

    int pc = pcs[0];
    PCSignature& sig = pcMap[pc];
    bool loop = isLoop(sig);
    bool strong = isStrong(sig);

    if (strong && loop) promoteToMRU(page);
    else if (strong && !loop) insertBeforeLRU(page);
    else touch(page);

    record.targetRow = targetRow;
    record.memorySnap = physicalFrames;
    history.push_back(record);

    // UI 출력용 데이터 스냅샷 저장
    StepPCExtra extra;
    extra.tick = tick;
    for (char p : lruList) extra.logicalLRU.push_back(p);
    
    for (char p : extra.logicalLRU) {
        bool protect = false;
        if (pageToPCs.count(p)) {
            for (int pcAddr : pageToPCs[p]) {
                if (isStrong(pcMap[pcAddr]) && isLoop(pcMap[pcAddr])) {
                    protect = true; break;
                }
            }
        }
        if (protect) extra.protectedPages.push_back(p);
    }
    
    for (const auto& pair : pcMap) {
        PCSigData d;
        d.pc = pair.first;
        d.weight = pair.second.faultWeight;
        d.historyStr = getHistoryString(pair.second);
        d.sigClass = getSignatureClass(pair.second);
        extra.activePCs.push_back(d);
    }
    pcExtras.push_back(extra);

    int step = hitCount + faultCount;
    float rate = (float)faultCount / step * 100.0f;
    timeData.push_back((float)step);
    faultRateData.push_back(rate);

    if (tick % K == 0) aging();
}