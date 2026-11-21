#pragma once
#include <minwindef.h>

struct CpuMetrics {
    ULONGLONG       cpuUsage;
    ULONGLONG       numOfProcesses;
    ULONGLONG       numOfSubProcesses;
    ULONGLONG       numOfIdentifiers;

    FLOAT           cpuSpeed;
    FLOAT           baseCpuSpeed;
    DWORD           numOfSockets;
    DWORD           numOfCores;
    DWORD           numOfLogicalProcessors;

    ULONGLONG       cacheL1;
    ULONGLONG       cacheL2;
    ULONGLONG       cacheL3;
};
