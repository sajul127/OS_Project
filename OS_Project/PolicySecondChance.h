#pragma once
#include "PolicyBase.h"

class PolicySecondChance : public PolicyBase {
private:
    std::vector<char> frames;    // 메인 메모리 페이지 배열
    std::vector<bool> refBits;   // 각 페이지의 참조 비트 (true: 1, false: 0)
    int currentCount = 0;        // 현재 채워진 페이지 개수
    int clockHand = 0;           // 교체 대상을 가리키는 시계 바늘 인덱스

public:
    PolicySecondChance(int size);
    void Operate(char data) override;
};