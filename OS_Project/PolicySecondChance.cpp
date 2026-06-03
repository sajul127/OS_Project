#include "PolicySecondChance.h"
#include <algorithm>

PolicySecondChance::PolicySecondChance(int size) : PolicyBase(size) {
    frames.resize(size, ' ');
    refBits.resize(size, false); // 초기 참조 비트는 모두 0(false)
}

void PolicySecondChance::Operate(char data) {
    StepRecord record;
    record.reqChar = data;

    // 1. Hit 검색
    auto it = std::find(frames.begin(), frames.end(), data);
    if (it != frames.end()) {
        record.status = PageStatus::Hit;
        int hitIndex = std::distance(frames.begin(), it);
        record.targetRow = hitIndex;

        // Hit 발생 시 참조 비트를 1(true)로 설정 (다시 기회 부여)
        refBits[hitIndex] = true;
        hitCount++;
    }
    else {
        // 2. Page Fault (빈 공간이 있는 경우)
        if (currentCount < frameSize) {
            record.status = PageStatus::Fault;

            frames[currentCount] = data;
            refBits[currentCount] = true; // 새로 메모리에 적재될 때 참조 비트를 1로 설정
            record.targetRow = currentCount;

            currentCount++;
            faultCount++;
        }
        // 3. Migration (빈 공간이 없어 밀어내야 하는 경우)
        else {
            record.status = PageStatus::Migration;

            // 희생자(Victim)를 찾을 때까지 시계 바늘 이동
            while (true) {
                if (refBits[clockHand] == true) {
                    // 기회를 한 번 더 주고(0으로 초기화), 바늘을 다음으로 이동
                    refBits[clockHand] = false;
                    clockHand = (clockHand + 1) % frameSize;
                }
                else {
                    // 참조 비트가 0인 페이지를 발견하면 교체 (희생자 당첨)
                    frames[clockHand] = data;
                    refBits[clockHand] = true; // 새로 삽입되었으므로 1로 설정
                    record.targetRow = clockHand;

                    // 페이지 교체 후 시계 바늘을 한 칸 다음으로 이동
                    clockHand = (clockHand + 1) % frameSize;
                    break;
                }
            }

            faultCount++;
            migrationCount++;
        }
    }

    // 현재 스냅샷 저장
    record.memorySnap = frames;
    record.refBitSnap = refBits; 
    record.clockHand = clockHand;
    history.push_back(record);

    int currentStep = hitCount + faultCount;
    float currentFaultRate = ((float)faultCount / currentStep) * 100.0f;

    timeData.push_back((float)currentStep);
    faultRateData.push_back(currentFaultRate);
}