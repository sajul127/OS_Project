#include "PolicyNUR.h"
#include <algorithm>
#include <cctype> // std::toupper, std::islower 포함

PolicyNUR::PolicyNUR(int size) : PolicyBase(size) {
    frames.resize(size, ' ');
    refBits.resize(size, false);
    modBits.resize(size, false);
}

void PolicyNUR::Operate(char data) {
    StepRecord record;
    record.reqChar = data;

    // 대문자는 '읽기', 소문자는 '쓰기'를 의미하고 항상 메모리에는 대문자로 저장
    char pageName = std::toupper(data);
    bool isWrite = std::islower(data);

    // 1. Hit 검색
    auto it = std::find(frames.begin(), frames.end(), pageName);
    if (it != frames.end()) {
        record.status = PageStatus::Hit;
        int hitIndex = std::distance(frames.begin(), it);
        record.targetRow = hitIndex;

        // Hit 발생 시 R 비트는 무조건 1, 쓰기 작업이었으면 M 비트도 1로 설정
        refBits[hitIndex] = true;
        if (isWrite) modBits[hitIndex] = true;

        hitCount++;
    }
    else {
        // 2. Page Fault (빈 공간이 있는 경우)
        if (currentCount < frameSize) {
            record.status = PageStatus::Fault;

            frames[currentCount] = pageName;
            refBits[currentCount] = true;         // 새로 삽입 시 R=1
            modBits[currentCount] = isWrite;      // 쓰기 요청이면 M=1, 아니면 M=0
            record.targetRow = currentCount;

            currentCount++;
            faultCount++;
        }
        // 3. Migration (빈 공간이 없어 교체해야 하는 경우)
        else {
            record.status = PageStatus::Migration;

            int victimRow = -1;
            int minClass = 4; // 클래스는 0~3까지 존재하므로 4로 초기화

            // clockHand부터 시작하여 각 페이지의 현재 상태 클래스(0~3)를 찾음
            for (int i = 0; i < frameSize; ++i) {
                int idx = (clockHand + i) % frameSize;

                // 클래스 계산: R이 1이면 +2, M이 1이면 +1
                // 0 (R=0, M=0) | 1 (R=0, M=1) | 2 (R=1, M=0) | 3 (R=1, M=1)
                int currentClass = (refBits[idx] ? 2 : 0) + (modBits[idx] ? 1 : 0);

                if (currentClass < minClass) {
                    minClass = currentClass;
                    victimRow = idx;
                    // 가장 낮은 클래스인 0을 찾았다면 더 찾을 필요 없이 바로 교체
                    if (minClass == 0) break;
                }
            }

            // 희생자(Victim) 페이지 교체
            frames[victimRow] = pageName;
            refBits[victimRow] = true;
            modBits[victimRow] = isWrite;
            record.targetRow = victimRow;

            // 페이지 교체 후 다음 희생자를 가리키도록 시계 핸들을 이동
            clockHand = (victimRow + 1) % frameSize;

            faultCount++;
            migrationCount++;
            
            // 3번의 Migration마다 모든 R 비트를 0으로 초기화
            if (migrationCount % 3 == 0) {
                std::fill(refBits.begin(), refBits.end(), false);
            }
            
        }
    }

    // 전체 요청(Hit + Fault)이 5번 발생할 때마다 모든 R비트를 0으로 초기화!
    int totalSteps = hitCount + faultCount;
    if (totalSteps > 0 && totalSteps % 5 == 0) {
        std::fill(refBits.begin(), refBits.end(), false); // 모든 R 비트를 0으로!
    }

    // 현재 스냅샷 저장
    record.memorySnap = frames;
    record.refBitSnap = refBits;
    record.modBitSnap = modBits;
    record.clockHand = clockHand;
    history.push_back(record);

    int currentStep = hitCount + faultCount;
    float currentFaultRate = ((float)faultCount / currentStep) * 100.0f;

    timeData.push_back((float)currentStep);
    faultRateData.push_back(currentFaultRate);
}