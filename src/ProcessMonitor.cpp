#include "../include/ProcessMonitor.hpp"
#include <handleapi.h>
#include <stringapiset.h>
#include <processthreadsapi.h>
#include <windef.h>
#include <winbase.h>
#include <chrono>
#include <thread>
#include <psapi.h>

static inline int getSizeToUtf8(const wchar_t* wcharArray, int len) {
    return WideCharToMultiByte(CP_UTF8, 0, wcharArray, len, nullptr, 0, nullptr, nullptr);
}

static inline std::string convertToUtf8(const wchar_t* wcharArray, int len) {
    std::string str = "";
    str.resize(len);
    WideCharToMultiByte(CP_UTF8, 0, wcharArray, -1, str.data(), len, nullptr, nullptr);
    return str;
}

static inline ULONGLONG fileTimeToU64(const FILETIME& ft) {
    ULARGE_INTEGER ul;
    ul.LowPart = ft.dwLowDateTime;
    ul.HighPart = ft.dwHighDateTime;
    return ul.QuadPart;
}

namespace {
constexpr auto kSampleInterval = std::chrono::milliseconds(100);

struct MetricSample {
    std::size_t index;
    HANDLE handle;
    ULONGLONG cpuTime;
    ULONGLONG diskBytes;
};
}

static void getProcessMetrics(std::vector<ProcessData>& pDataVector)
{
    FILETIME creationTime{};
    FILETIME exitTime{};
    FILETIME idle1{};
    FILETIME kernel1{};
    FILETIME user1{};
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    DWORD numLogicalCores = sysInfo.dwNumberOfProcessors;

    GetSystemTimes(&idle1, &kernel1, &user1);
    const ULONGLONG systemBusy1 = fileTimeToU64(kernel1) + fileTimeToU64(user1) - fileTimeToU64(idle1);

    std::vector<MetricSample> samples;
    samples.reserve(pDataVector.size());

    for (std::size_t i = 0; i < pDataVector.size(); ++i) {
        ProcessData& pData = pDataVector[i];
        HANDLE procHandle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pData.processId);
        if (procHandle == NULL) {
            pData.metricsAvailable = false;
            continue;
        }

        FILETIME kernelTime1{};
        FILETIME userTime1{};
        IO_COUNTERS io1{};
        if (GetProcessTimes(procHandle, &creationTime, &exitTime, &kernelTime1, &userTime1) &&
            GetProcessIoCounters(procHandle, &io1)) {
            samples.push_back({i, procHandle,
                               fileTimeToU64(kernelTime1) + fileTimeToU64(userTime1),
                               io1.ReadTransferCount + io1.WriteTransferCount});
        } else {
            pData.metricsAvailable = false;
            CloseHandle(procHandle);
        }
    }

    const auto t1 = std::chrono::steady_clock::now();
    std::this_thread::sleep_for(kSampleInterval);
    const auto t2 = std::chrono::steady_clock::now();
    const double elapsedSeconds = std::chrono::duration<double>(t2 - t1).count();

    FILETIME idle2{};
    FILETIME kernel2{};
    FILETIME user2{};
    GetSystemTimes(&idle2, &kernel2, &user2);
    const ULONGLONG systemBusy2 =
        fileTimeToU64(kernel2) + fileTimeToU64(user2) - fileTimeToU64(idle2);
    const ULONGLONG systemDelta = systemBusy2 > systemBusy1 ? systemBusy2 - systemBusy1 : 0;

    for (const MetricSample& sample : samples) {
        ProcessData& pData = pDataVector[sample.index];

        FILETIME kernelTime2{};
        FILETIME userTime2{};
        if (GetProcessTimes(sample.handle, &creationTime, &exitTime, &kernelTime2, &userTime2)) {
            const ULONGLONG cpuTime2 = fileTimeToU64(kernelTime2) + fileTimeToU64(userTime2);
            const ULONGLONG cpuDelta = cpuTime2 > sample.cpuTime ? cpuTime2 - sample.cpuTime : 0;
            pData.cpuUsage = systemDelta > 0
                ? static_cast<ULONG>((static_cast<double>(cpuDelta) / (systemDelta * numLogicalCores)) * 100.0)
                : 0;
        }

        IO_COUNTERS io2{};
        if (GetProcessIoCounters(sample.handle, &io2)) {
            const ULONGLONG diskBytes2 = io2.ReadTransferCount + io2.WriteTransferCount;
            const ULONGLONG diskDelta = diskBytes2 > sample.diskBytes ? diskBytes2 - sample.diskBytes : 0;
            pData.diskUsage = elapsedSeconds > 0
                ? static_cast<FLOAT>(diskDelta / elapsedSeconds)
                : 0.0f;
        }

        PROCESS_MEMORY_COUNTERS pmc{};
        pmc.cb = sizeof(pmc);
        if (GetProcessMemoryInfo(sample.handle, &pmc, sizeof(pmc))) {
            pData.memWorkingSet = static_cast<ULONG>(pmc.WorkingSetSize / (1024.0 * 1024.0));
        }

        CloseHandle(sample.handle);
    }
}

void ProcessMonitor::startPolling() noexcept {
    do {
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return;

        PROCESSENTRY32 procEntry{};
        procEntry.dwSize = sizeof(procEntry);
        std::vector<ProcessData> procSnapshots;

        if (Process32First(snapshot, &procEntry)) {
            do {
                int size = getSizeToUtf8(procEntry.szExeFile, PATH_MAX);
                procSnapshots.emplace_back(convertToUtf8(procEntry.szExeFile, size), procEntry.th32ProcessID, procEntry.cntThreads);

            } while (Process32Next(snapshot, &procEntry));
        }
        CloseHandle(snapshot);
        getProcessMetrics(procSnapshots);
        m_pQueue->push(std::move(procSnapshots));
        Sleep(m_pollInterval);
    } while (m_monitorIsRunning);
}

void ProcessMonitor::stopPolling() noexcept {
    m_monitorIsRunning = false;
}

void ProcessMonitor::setProcessQueue(ProcessQueue* p) noexcept {m_pQueue = p;}
