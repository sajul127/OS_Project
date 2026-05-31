#pragma once
#include "PolicyBase.h"
#include <vector>

class PolicyNUR : public PolicyBase {
private:
    std::vector<char> frames;
    std::vector<bool> refBits; // 참조 비트 (R)
    std::vector<bool> modBits; // 변형 비트 (M)
    int currentCount = 0;
    int clockHand = 0;         // 희생자 탐색을 위한 시작점 (공평성을 위해)

public:
    PolicyNUR(int size);
    virtual void Operate(char data) override;
};