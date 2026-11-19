#pragma once
#include <string>
#include <vector>
#include <minwindef.h>
#include <winnt.h>
#include <tlhelp32.h>
#include "ProcessData.hpp"
#include "ProcessQueue.hpp"

class ProcessMonitor {
private:
    ProcessQueue*               m_pQueue;
    ULONG                       m_pollInterval;
    bool                        m_monitorIsRunning;

public:
    ProcessMonitor(ULONG pollIntervalMs) noexcept : m_pollInterval(pollIntervalMs), 
    m_monitorIsRunning(true) {};
    void startPolling() noexcept;
    void stopPolling() noexcept;
    void setProcessQueue(ProcessQueue* p) noexcept;
};
