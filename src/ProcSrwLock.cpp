#include "../include/ProcSrwLock.hpp"

void ProcSrwLock::acquireSharedLock() noexcept {
    AcquireSRWLockShared(&m_lock);
    m_slocked = true;
}

void ProcSrwLock::acquireExclusiveLock() noexcept {
    AcquireSRWLockExclusive(&m_lock);
    m_elocked = true;
}

void ProcSrwLock::releaseSharedLock() noexcept {
    ReleaseSRWLockShared(&m_lock);
    m_slocked = false;
}

void ProcSrwLock::releaseExclusiveLock() noexcept {
    ReleaseSRWLockExclusive(&m_lock);
    m_elocked = false;
}

SRWLOCK& ProcSrwLock::getRawLock() noexcept {
    return m_lock;
}
