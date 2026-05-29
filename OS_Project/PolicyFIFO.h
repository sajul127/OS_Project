#pragma once
#include "PolicyBase.h"
#include <queue>

class PolicyFifo : public PolicyBase {
private:
    std::vector<char> frames;    // 실제 메모리 프레임 배열 (크기: frameSize)
    std::queue<int> fifoQueue;   // 교체될 프레임의 '인덱스'를 관리하는 큐
    int currentCount = 0;        // 현재 채워진 프레임 개수

public:
    PolicyFifo(int size);
    void Operate(char data) override;
};