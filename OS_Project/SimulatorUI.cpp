#include "SimulatorUI.h"
#include "PolicyFifo.h"
#include "PolicySecondChance.h"
#include "PolicyNUR.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#include <string>
#include <cstdlib> 
#include <ctime>   
#include <cctype>

void SimulatorUI::Initialize(GLFWwindow* window) {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    std::srand((unsigned int)std::time(nullptr)); 
}

void SimulatorUI::Render() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // 현재 프로그램 전체 화면 해상도 동적 획득
    ImGuiIO& io = ImGui::GetIO();
    float displayWidth = io.DisplaySize.x;
    float displayHeight = io.DisplaySize.y;

    // 고정형 창 플래그 설정
    ImGuiWindowFlags fixedFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

    // -----------------------------------------------------------
    // 1. 제어 패널 (상단 영역)
    // -----------------------------------------------------------
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(displayWidth, 65));
    ImGui::Begin("Simulator Control", nullptr, fixedFlags | ImGuiWindowFlags_NoTitleBar);

    const char* policies[] = { "FIFO", "LRU Second Chance", "LRU NUR", "LRU PC" };
    ImGui::SetNextItemWidth(150);
    ImGui::Combo("Policy", &currentPolicyIdx, policies, IM_ARRAYSIZE(policies));

    ImGui::SameLine(0, 25);
    ImGui::SetNextItemWidth(350);
    ImGui::InputText("Reference String", refString, IM_ARRAYSIZE(refString));

    ImGui::SameLine(0, 8);
    if (ImGui::Button("Random", ImVec2(70, 0))) {
        int count = 10 + (std::rand() % 21);  // 10 ~ 30개의 길이
        for (int i = 0; i < count; i++) {
            char randomChar = 'A' + (std::rand() % 6);
            if (std::rand() % 100 < 30) {
                randomChar = std::tolower(randomChar);
            }
            refString[i] = randomChar;
        }
        refString[count] = '\0';
    }

    // ⭐ 추가된 부분 1: 입력 힌트 안내문
    ImGui::SameLine(0, 10);
    ImGui::TextDisabled("(A-F: Read, a-f: Write)");

    ImGui::SameLine(0, 25);
    ImGui::SetNextItemWidth(80);
    ImGui::InputInt("#Frame", &frameSize);

    ImGui::SameLine(0, 25);
    if (ImGui::Button("Run Simulation", ImVec2(130, 0))) {
        if (currentPolicy) { delete currentPolicy; currentPolicy = nullptr; }

        switch (currentPolicyIdx) {
        case 0: currentPolicy = new PolicyFifo(frameSize); break;
        case 1: currentPolicy = new PolicySecondChance(frameSize); break;
        case 2: currentPolicy = new PolicyNUR(frameSize); break;
        default: currentPolicy = new PolicyFifo(frameSize); break;
        }

        std::string ref(refString);
        for (char c : ref) {
            if (c != '\0' && c != ' ') {
                currentPolicy->Operate(c);
            }
        }
        isSimulated = true;
    }
    ImGui::End();

    // -----------------------------------------------------------
    // 2. 데이터 출력 영역 (그리드 & 통계)
    // -----------------------------------------------------------
    if (isSimulated && currentPolicy) {
        const auto& history = currentPolicy->GetHistory();
        int totalCols = (int)history.size() + 1;
        int activeFrameSize = currentPolicy->GetFrameSize();

        float gridWidth = displayWidth * 0.65f;
        float sidebarWidth = displayWidth * 0.35f;
        float contentHeight = displayHeight - 65.0f;

        // --- 좌측 패널: 메모리 프레임 그리드 ---
        ImGui::SetNextWindowPos(ImVec2(0, 65));
        ImGui::SetNextWindowSize(ImVec2(gridWidth, contentHeight));
        ImGui::Begin("Memory Frame Grid", nullptr, fixedFlags);

        // ⭐ 추가된 부분 2: 색상 범례(Legend) 및 NUR 클래스 설명
        ImGui::Text("Legend: ");
        ImGui::SameLine();
        ImGui::ColorButton("HitColor", ImVec4(50 / 255.f, 205 / 255.f, 50 / 255.f, 1.0f)); ImGui::SameLine(); ImGui::Text("Hit"); ImGui::SameLine(0, 15);
        ImGui::ColorButton("FaultColor", ImVec4(255 / 255.f, 0, 0, 1.0f)); ImGui::SameLine(); ImGui::Text("Fault"); ImGui::SameLine(0, 15);
        ImGui::ColorButton("MigrateColor", ImVec4(128 / 255.f, 0, 128 / 255.f, 1.0f)); ImGui::SameLine(); ImGui::Text("Migration"); ImGui::SameLine(0, 15);
        ImGui::ColorButton("HandColor", ImVec4(200 / 255.f, 200 / 255.f, 50 / 255.f, 100 / 255.f)); ImGui::SameLine(); ImGui::Text("Clock Hand");

        if (currentPolicyIdx == 2) {
            ImGui::TextDisabled("NUR Priority: Class 0 (R:0 M:0, White) > Class 1 (R:0 M:1, Cyan) > Class 2 (R:1 M:0, Yellow) > Class 3 (R:1 M:1, Red)");
        }
        ImGui::Spacing(); // 표와의 간격 벌리기

        if (ImGui::BeginTable("GridTable", totalCols, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollX | ImGuiTableFlags_ScrollY)) {

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Ref");
            for (const auto& step : history) {
                ImGui::TableNextColumn();
                ImGui::Text(" %c ", step.reqChar);
            }

            for (int r = 0; r < activeFrameSize; ++r) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("F%d", r);

                for (const auto& step : history) {
                    ImGui::TableNextColumn();

                    ImU32 bgColor = 0;
                    if (step.targetRow == r) {
                        if (step.status == PageStatus::Hit)       bgColor = IM_COL32(50, 205, 50, 255);
                        else if (step.status == PageStatus::Fault)bgColor = IM_COL32(255, 0, 0, 255);
                        else                                      bgColor = IM_COL32(128, 0, 128, 255);

                        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, bgColor);
                    }

                    // ⭐ 수정된 부분 3: Second Chance(1)와 NUR(2) 모두 시계 바늘 표시
                    if ((currentPolicyIdx == 1 || currentPolicyIdx == 2) && step.clockHand == r && step.targetRow != r) {
                        bgColor = IM_COL32(200, 200, 50, 100); // 옅은 노란색 반투명
                    }

                    if (bgColor != 0) {
                        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, bgColor);
                    }

                    char cellData = step.memorySnap[r];
                    if (cellData != ' ') {
                        if (currentPolicyIdx == 1 && !step.refBitSnap.empty()) {
                            int refBit = step.refBitSnap[r] ? 1 : 0;
                            if (refBit == 1) ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "%c (1)", cellData);
                            else ImGui::Text(" %c (0)", cellData);
                        }
                        else if (currentPolicyIdx == 2 && !step.modBitSnap.empty()) {
                            int rBit = step.refBitSnap[r] ? 1 : 0;
                            int mBit = step.modBitSnap[r] ? 1 : 0;

                            if (rBit == 1 && mBit == 1)      ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "%c (R:1 M:1)", cellData);
                            else if (rBit == 1 && mBit == 0) ImGui::TextColored(ImVec4(1, 1, 0.5f, 1), "%c (R:1 M:0)", cellData);
                            else if (rBit == 0 && mBit == 1) ImGui::TextColored(ImVec4(0.5f, 1, 1, 1), "%c (R:0 M:1)", cellData);
                            else                             ImGui::Text(" %c (R:0 M:0)", cellData);
                        }
                        else {
                            ImGui::Text(" %c ", cellData);
                        }
                    }
                }
            }
            ImGui::EndTable();
        }
        ImGui::End();

        // --- 우측 패널: 통계 및 콘솔 로그 ---
        ImGui::SetNextWindowPos(ImVec2(gridWidth, 65));
        ImGui::SetNextWindowSize(ImVec2(sidebarWidth, contentHeight));
        ImGui::Begin("Statistics & Logs", nullptr, fixedFlags);

        ImGui::TextColored(ImVec4(0.5f, 1.0f, 0.5f, 1.0f), "Hits: %d", currentPolicy->GetHit());
        ImGui::SameLine(90);
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "Faults: %d", currentPolicy->GetFault());
        ImGui::SameLine(190);
        ImGui::TextColored(ImVec4(0.8f, 0.5f, 1.0f, 1.0f), "Migrated: %d", currentPolicy->GetMigration());

        int totalReq = currentPolicy->GetHit() + currentPolicy->GetFault();
        float finalRate = (totalReq > 0) ? ((float)currentPolicy->GetFault() / totalReq) * 100.0f : 0.0f;
        ImGui::Text("Page Fault Rate = %.1f%%", finalRate);

        ImGui::Separator();

        if (ImPlot::BeginPlot("Fault Rate Trend", ImVec2(-1, contentHeight * 0.4f))) {
            ImPlot::SetupAxes("Step", "Fault Rate (%)");
            ImPlot::SetupAxesLimits(0, totalReq + 1, 0, 110);

            const auto& tData = currentPolicy->GetTimeData();
            const auto& fData = currentPolicy->GetFaultRateData();
            ImPlot::PlotLine("Fault Rate", tData.data(), fData.data(), (int)tData.size());
            ImPlot::EndPlot();
        }

        ImGui::Separator();
        ImGui::Text("Console Log");

        ImGui::BeginChild("ScrollingRegion", ImVec2(0, -1), true, ImGuiWindowFlags_HorizontalScrollbar);
        for (const auto& step : history) {
            const char* statusStr = (step.status == PageStatus::Hit) ? "Hit" :
                (step.status == PageStatus::Fault) ? "Page Fault" : "Migrated";
            ImGui::Text("DATA %c is %s", step.reqChar, statusStr);
        }
        ImGui::SetScrollHereY(1.0f);
        ImGui::EndChild();

        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void SimulatorUI::Shutdown() {
    if (currentPolicy) { delete currentPolicy; currentPolicy = nullptr; }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}