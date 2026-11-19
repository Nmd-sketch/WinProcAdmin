#pragma once
#include <synchapi.h>
#include <minwindef.h>


class ProcSrwLock {
private:
    SRWLOCK     m_lock;
    bool        m_slocked;
    bool        m_elocked;

public:
    ProcSrwLock() : m_lock(SRWLOCK_INIT), m_slocked(false), m_elocked(false) {}
    ProcSrwLock(const ProcSrwLock&) = delete;
    ProcSrwLock(ProcSrwLock&&) = delete;
    
    ProcSrwLock& operator=(const ProcSrwLock&) = delete;
    ProcSrwLock& operator=(ProcSrwLock&&) = delete;

    ~ProcSrwLock() {
        if (m_slocked) {
            releaseSharedLock();
        } else if (m_elocked) {
            releaseExclusiveLock();
        }
    }
    
    void acquireSharedLock() noexcept;
    void acquireExclusiveLock() noexcept;
    void releaseSharedLock() noexcept;
    void releaseExclusiveLock() noexcept;
    SRWLOCK& getRawLock() noexcept;
};
