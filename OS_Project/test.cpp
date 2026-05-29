#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "thirdparty/lib-vc2022/glfw3.lib") 

#include <GLFW/glfw3.h>
#include "SimulatorUI.h"

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Page Replacement Simulator", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // VSync

    // 우리가 만든 UI 클래스 초기화
    SimulatorUI ui;
    ui.Initialize(window);

    // 메인 루프
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 여기서 모든 연산과 그리기 처리가 일어남
        ui.Render();

        glfwSwapBuffers(window);
    }

    ui.Shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}