#pragma once
#include "PolicyBase.h"
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <vector>
#include <string>

// PC 1개의 상태
struct PCSignature {
    int lastAccessTick = 0;
    int faultWeight = 0;
    std::vector<char> history;
    int head = 0;
    std::unordered_set<char> accessedPages;
};

// UI 대시보드 출력을 위한 PC 데이터 스냅샷
struct PCSigData {
    int pc;
    std::string historyStr;
    int weight;
    std::string sigClass;
};

// 매 Step마다 UI로 넘겨줄 PC 전용 데이터
struct StepPCExtra {
    std::vector<PCSigData> activePCs;
    std::vector<char> protectedPages;
    std::vector<char> logicalLRU; 
    int tick;
};

class PolicyPCSignature : public PolicyBase {
private:
    int N, K, threshold;
    int tick = 0;
    
    std::unordered_map<int, PCSignature> pcMap;
    std::unordered_map<char, std::unordered_set<int>> pageToPCs;
    
    std::list<char> lruList;
    std::unordered_map<char, std::list<char>::iterator> lruPos;
    std::unordered_set<char> frames;
    
    std::vector<char> physicalFrames; 
    std::vector<StepPCExtra> pcExtras;

    void updateHistory(PCSignature& sig, char page);
    bool isLoop(const PCSignature& sig);
    bool isStrong(const PCSignature& sig);
    void touch(char page);
    void promoteToMRU(char page);
    void insertBeforeLRU(char page);
    char selectVictim();
    void aging();
    
    std::string getHistoryString(const PCSignature& sig);
    std::string getSignatureClass(const PCSignature& sig);

public:
    PolicyPCSignature(int size, int n = 4, int k = 1000, int th = 4);
    void Operate(char page) override;
    void OperateWithPCs(char page, const std::vector<int>& pcs) override;
    
    const StepPCExtra& GetPCExtra(int stepIndex) const { return pcExtras[stepIndex]; }
    int GetAgingPeriod() const { return K; }
};