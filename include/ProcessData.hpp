#pragma once
#include <minwindef.h>
#include <array>
#include <vector>

struct ProcessData {
    std::array<char, MAX_PATH>    processName;
    ULONG                         processId;
    ULONG                         numberOfThreads;
    ULONG                         memWorkingSet;
    ULONG                         cpuUsage;
    FLOAT                         diskUsage;
    bool                          metricsAvailable = true;
};

struct ProcessSnapshot {
    std::vector<ProcessData>    procDataBuf;
};
