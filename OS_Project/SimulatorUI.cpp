#include "SimulatorUI.h"
#include "PolicyFifo.h"
#include "PolicySecondChance.h"
#include "PolicyNUR.h"
#include "PolicyPCSignature.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "implot.h"

#include <string>
#include <cstdlib> 
#include <ctime>   
#include <cctype>
#include <vector>
#include <sstream>

// 클래스 외부 정적 헬퍼 함수
static std::vector<std::pair<char, std::vector<int>>> ParseInput(const std::string& input) {
    std::vector<std::pair<char, std::vector<int>>> result;
    size_t start = 0;
    while (start < input.size()) {
        while (start < input.size() && input[start] == ' ') start++;
        size_t colon = input.find(':', start);
        if (colon == std::string::npos) break;
        char page = input[start];
        size_t end = input.find(';', colon);
        if (end == std::string::npos) end = input.size();
        std::string pcsStr = input.substr(colon + 1, end - colon - 1);
        
        std::vector<int> pcs;
        std::stringstream ss(pcsStr);
        std::string token;
        while (std::getline(ss, token, ',')) {
            if (!token.empty()) pcs.push_back(std::stoi(token));
        }
        result.push_back({ page, pcs });
        start = end + 1;
    }
    return result;
}

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

    ImGuiIO& io = ImGui::GetIO();
    float displayWidth = io.DisplaySize.x;
    float displayHeight = io.DisplaySize.y;
    ImGuiWindowFlags fixedFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

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
        int count = 10 + (std::rand() % 21);
        
        if (currentPolicyIdx == 3) {
            std::string result;
            for (int i = 0; i < count; i++) {
                char page = 'A' + (std::rand() % 6);
                result += page;
                result += ":";
                int pcCount = 1 + std::rand() % 2;
                for (int j = 0; j < pcCount; j++) {
                    result += std::to_string(100 + std::rand() % 900);
                    if (j < pcCount - 1) result += ",";
                }
                if (i < count - 1) result += "; ";
            }
            strcpy_s(refString, sizeof(refString), result.c_str());
        }
        else {
            for (int i = 0; i < count; i++) {
                char randomChar = 'A' + (std::rand() % 6);
                if (std::rand() % 100 < 30) randomChar = std::tolower(randomChar);
                refString[i] = randomChar;
            }
            refString[count] = '\0';
        }
    }

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
        case 3: currentPolicy = new PolicyPCSignature(frameSize); break;
        default: currentPolicy = new PolicyFifo(frameSize); break;
        }

        std::string input(refString);
        bool isPCInput = (input.find(':') != std::string::npos);

        if (currentPolicyIdx == 3) {
            if (isPCInput) {
                auto parsed = ParseInput(input);
                for (auto& p : parsed) {
                    currentPolicy->OperateWithPCs(p.first, p.second);
                }
            } else {
                for (char c : input) {
                    if (c == ' ' || c == '\0') continue;
                    std::vector<int> pcs;
                    int pcCount = 1 + std::rand() % 2;
                    for (int i = 0; i < pcCount; i++) pcs.push_back(100 + std::rand() % 900);
                    currentPolicy->OperateWithPCs(c, pcs);
                }
            }
        } else {
            for (char c : input) {
                if (c != ' ' && c != '\0') currentPolicy->Operate(c);
            }
        }
        isSimulated = true;
    }
    ImGui::End();

    if (isSimulated && currentPolicy) {
        const auto& history = currentPolicy->GetHistory();
        int totalCols = (int)history.size() + 1;
        int activeFrameSize = currentPolicy->GetFrameSize();

        PolicyPCSignature* pcPolicy = nullptr;
        if (currentPolicyIdx == 3) pcPolicy = dynamic_cast<PolicyPCSignature*>(currentPolicy);

        float gridWidth = displayWidth * 0.65f;
        float sidebarWidth = displayWidth * 0.35f;
        float contentHeight = displayHeight - 65.0f;

        ImGui::SetNextWindowPos(ImVec2(0, 65));
        ImGui::SetNextWindowSize(ImVec2(gridWidth, contentHeight));
        ImGui::Begin("Memory Frame Grid", nullptr, fixedFlags);

        ImGui::Text("Legend: ");
        ImGui::SameLine();
        ImGui::ColorButton("HitColor", ImVec4(50 / 255.f, 205 / 255.f, 50 / 255.f, 1.0f)); ImGui::SameLine(); ImGui::Text("Hit"); ImGui::SameLine(0, 15);
        ImGui::ColorButton("FaultColor", ImVec4(255 / 255.f, 0, 0, 1.0f)); ImGui::SameLine(); ImGui::Text("Fault"); ImGui::SameLine(0, 15);
        ImGui::ColorButton("MigrateColor", ImVec4(128 / 255.f, 0, 128 / 255.f, 1.0f)); ImGui::SameLine(); ImGui::Text("Migration"); ImGui::SameLine(0, 15);
        
        if (currentPolicyIdx == 1 || currentPolicyIdx == 2) {
            ImGui::ColorButton("HandColor", ImVec4(200 / 255.f, 200 / 255.f, 50 / 255.f, 100 / 255.f)); ImGui::SameLine(); ImGui::Text("Clock Hand");
        }
        if (currentPolicyIdx == 3) {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "[P] Protected by Strong Loop PC");
        }
        
        if (currentPolicyIdx == 2) {
            ImGui::TextDisabled("NUR Priority: Class 0 (R:0 M:0, White) > Class 1 (R:0 M:1, Cyan) > Class 2 (R:1 M:0, Yellow) > Class 3 (R:1 M:1, Red)");
        }
        ImGui::Spacing();

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

                int stepIndex = 0;
                for (const auto& step : history) {
                    ImGui::TableNextColumn();

                    ImU32 bgColor = 0;
                    if (step.targetRow == r) {
                        if (step.status == PageStatus::Hit)       bgColor = IM_COL32(50, 205, 50, 255);
                        else if (step.status == PageStatus::Fault)bgColor = IM_COL32(255, 0, 0, 255);
                        else                                      bgColor = IM_COL32(128, 0, 128, 255);

                        ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, bgColor);
                    }

                    if ((currentPolicyIdx == 1 || currentPolicyIdx == 2) && step.clockHand == r && step.targetRow != r) {
                        bgColor = IM_COL32(200, 200, 50, 100); 
                    }

                    if (bgColor != 0) ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, bgColor);

                    char cellData = step.memorySnap[r];
                    if (cellData != ' ') {
                        if (currentPolicyIdx == 3 && pcPolicy != nullptr) {
                            const auto& extra = pcPolicy->GetPCExtra(stepIndex);
                            bool isProtected = std::find(extra.protectedPages.begin(), extra.protectedPages.end(), cellData) != extra.protectedPages.end();
                            
                            if (isProtected) ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "%c [P]", cellData);
                            else ImGui::Text(" %c ", cellData);
                        }
                        else if (currentPolicyIdx == 1 && !step.refBitSnap.empty()) {
                            int refBit = step.refBitSnap[r] ? 1 : 0;
                            if (refBit == 1) ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.5f, 1.0f), "%c (1)", cellData);
                            else ImGui::Text(" %c (0)", cellData);
                        }
                        else if (currentPolicyIdx == 2 && !step.modBitSnap.empty()) {
                            int rBit = step.refBitSnap[r] ? 1 : 0;
                            int mBit = step.modBitSnap[r] ? 1 : 0;
                            if (rBit == 1 && mBit == 1)      ImGui::TextColored(ImVec4(1, 0.5f, 0.5f, 1), "%c (1,1)", cellData);
                            else if (rBit == 1 && mBit == 0) ImGui::TextColored(ImVec4(1, 1, 0.5f, 1), "%c (1,0)", cellData);
                            else if (rBit == 0 && mBit == 1) ImGui::TextColored(ImVec4(0.5f, 1, 1, 1), "%c (0,1)", cellData);
                            else                             ImGui::Text(" %c (0,0)", cellData);
                        }
                        else {
                            ImGui::Text(" %c ", cellData);
                        }
                    }
                    stepIndex++;
                }
            }
            ImGui::EndTable();
        }
        ImGui::End();

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

        if (currentPolicyIdx == 3 && pcPolicy != nullptr && !history.empty()) {
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "[ PC Signature Dashboard ]");
            
            const auto& extra = pcPolicy->GetPCExtra(history.size() - 1);
            float agingProgress = (float)(extra.tick % pcPolicy->GetAgingPeriod()) / pcPolicy->GetAgingPeriod();
            ImGui::ProgressBar(agingProgress, ImVec2(-1.0f, 0.0f), "Aging Cycle Progress");
            
            ImGui::Text("Logical LRU Queue (MRU -> LRU):");
            std::string queueStr;
            for (char p : extra.logicalLRU) queueStr += std::string(1, p) + " -> ";
            queueStr += "EVICT";
            ImGui::TextWrapped("%s", queueStr.c_str());
            ImGui::Spacing();
            
            ImGui::Text("Active PC Tracking:");
            if (ImGui::BeginTable("PCTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 160))) {
                ImGui::TableSetupColumn("PC"); ImGui::TableSetupColumn("History"); ImGui::TableSetupColumn("W"); ImGui::TableSetupColumn("Class");
                ImGui::TableHeadersRow();
                for (const auto& pc : extra.activePCs) {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn(); ImGui::Text("%d", pc.pc);
                    ImGui::TableNextColumn(); ImGui::Text("%s", pc.historyStr.c_str());
                    ImGui::TableNextColumn(); ImGui::Text("%d", pc.weight);
                    ImGui::TableNextColumn(); 
                    if (pc.sigClass == "Strong Loop") ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", pc.sigClass.c_str());
                    else if (pc.sigClass == "Strong Scan") ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.0f, 1.0f), "%s", pc.sigClass.c_str());
                    else if (pc.sigClass == "Weak Loop") ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%s", pc.sigClass.c_str());
                    else ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "%s", pc.sigClass.c_str());
                }
                ImGui::EndTable();
            }
        }

        ImGui::Separator();
        float plotHeight = (currentPolicyIdx == 3) ? contentHeight * 0.2f : contentHeight * 0.4f;
        if (ImPlot::BeginPlot("Fault Rate Trend", ImVec2(-1, plotHeight))) {
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
            const char* statusStr = (step.status == PageStatus::Hit) ? "Hit" : (step.status == PageStatus::Fault) ? "Page Fault" : "Migrated";
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