#pragma once
#include <GLFW/glfw3.h>
#include "PolicyBase.h"

class SimulatorUI {
private:
    char refString[256] = "123412512345";
    int frameSize = 4;
    int currentPolicyIdx = 0;
    bool isSimulated = false;

    // 다형성을 위한 부모 포인터 (어떤 알고리즘이든 담을 수 있음)
    PolicyBase* currentPolicy = nullptr; 

public:
    void Initialize(GLFWwindow* window);
    void Render();
    void Shutdown();
};