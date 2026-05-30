#pragma once
#include "PolicyBase.h"

class PolicySecondChance : public PolicyBase {
private:
    std::vector<char> frames;    // 실제 메모리 프레임 배열
    std::vector<bool> refBits;   // 각 프레임의 참조 비트 (true: 1, false: 0)
    int currentCount = 0;        // 현재 채워진 프레임 개수
    int clockHand = 0;           // 교체 대상을 가리키는 시계 바늘 인덱스

public:
    PolicySecondChance(int size);
    void Operate(char data) override;
};