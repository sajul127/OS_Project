#include "PolicyFifo.h"
#include <algorithm>

PolicyFifo::PolicyFifo(int size) : PolicyBase(size) {
    frames.resize(size, ' '); // 공백으로 초기화
}

void PolicyFifo::Operate(char data) {
    StepRecord record;
    record.reqChar = data;

    // 1. Hit 검사
    auto it = std::find(frames.begin(), frames.end(), data);
    if (it != frames.end()) {
        record.status = PageStatus::Hit;
        record.targetRow = std::distance(frames.begin(), it); // Hit된 위치
        hitCount++;
    } 
    else {
        // 2. Page Fault (빈 공간이 있는 경우)
        if (currentCount < frameSize) {
            record.status = PageStatus::Fault;
            frames[currentCount] = data;
            record.targetRow = currentCount;
            fifoQueue.push(currentCount); // 들어간 위치를 큐에 기록
            
            currentCount++;
            faultCount++;
        } 
        // 3. Migration (빈 공간이 없어 쫓아내야 하는 경우)
        else {
            record.status = PageStatus::Migration;
            
            int victimRow = fifoQueue.front(); // 가장 먼저 들어온 위치를 뽑음
            fifoQueue.pop();
            
            frames[victimRow] = data;          // 해당 위치의 데이터 교체
            record.targetRow = victimRow;
            fifoQueue.push(victimRow);         // 다시 큐의 맨 뒤로
            
            faultCount++;
            migrationCount++;
        }
    }

    // 스냅샷 및 차트 데이터 저장
    record.memorySnap = frames;
    history.push_back(record);

    int currentStep = hitCount + faultCount;
    float currentFaultRate = ((float)faultCount / currentStep) * 100.0f;
    
    timeData.push_back((float)currentStep);
    faultRateData.push_back(currentFaultRate);
}