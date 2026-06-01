#pragma once
#include <vector>

enum class PageStatus {
    Hit,
    Fault,
    Migration
};

struct StepRecord {
    char reqChar = ' ';
    PageStatus status = PageStatus::Fault;            
    int targetRow = -1;                
    std::vector<char> memorySnap;
    std::vector<bool> refBitSnap; 
    std::vector<bool> modBitSnap;
    int clockHand = 0;                
};

class PolicyBase {
protected:
    int frameSize;
    int hitCount = 0;
    int faultCount = 0;
    int migrationCount = 0;

    std::vector<StepRecord> history; 
    std::vector<float> timeData;
    std::vector<float> faultRateData;

public:
    PolicyBase(int size) : frameSize(size) {}
    virtual ~PolicyBase() = default;

    virtual void Operate(char data) = 0; 
    
    virtual void OperateWithPCs(char data, const std::vector<int>& pcs) {
        Operate(data); 
    }

    int GetHit() const { return hitCount; }
    int GetFault() const { return faultCount; }
    int GetMigration() const { return migrationCount; }
    int GetFrameSize() const { return frameSize; } 
    const std::vector<StepRecord>& GetHistory() const { return history; }
    const std::vector<float>& GetTimeData() const { return timeData; }
    const std::vector<float>& GetFaultRateData() const { return faultRateData; }
};