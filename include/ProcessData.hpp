#pragma once
#include <minwindef.h>
#include <string>

struct ProcessData {
    std::string              processName;
    ULONG                    processId;
    ULONG                    numberOfThreads;
    ULONG                    memWorkingSet;
    ULONG                    cpuUsage;
    FLOAT                    diskUsage;
    bool                     metricsAvailable = true;
};
