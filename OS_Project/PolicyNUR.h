#pragma once
#include "PolicyBase.h"
#include <vector>

class PolicyNUR : public PolicyBase {
private:
    std::vector<char> frames;
    std::vector<bool> refBits; // 참조 비트 (R)
    std::vector<bool> modBits; // 수정 비트 (M)
    int currentCount = 0;
    int clockHand = 0;         // 클럭 핸드가 가리키는 페이지 (시계 방향)

public:
    PolicyNUR(int size);
    virtual void Operate(char data) override;
};