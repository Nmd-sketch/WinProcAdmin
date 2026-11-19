#include "../include/ProcessQueue.hpp"

#ifndef INFINITE
#define INFINITE 0xffffffff
#endif

ProcessQueue::ProcessQueue(ssize_t maxDequeSize)
    : m_closed(false), max_size(maxDequeSize)
{
    InitializeConditionVariable(&m_cv);
}

void ProcessQueue::push(ProcessSnapshot snap) noexcept {
    m_lock.acquireExclusiveLock();
    if (m_closed) {
        m_lock.releaseExclusiveLock();
        return;
    }

    m_deque.push_back(std::move(snap));
    if (m_deque.size() > static_cast<std::size_t>(max_size)) {
        m_deque.pop_front();
    }
    m_lock.releaseExclusiveLock();

    WakeConditionVariable(&m_cv);
}

bool ProcessQueue::try_pop() {
    m_lock.acquireExclusiveLock();
    const bool popped = !m_deque.empty();
    if (popped) {
        m_deque.pop_front();
    }
    m_lock.releaseExclusiveLock();
    return popped;
}

ProcessSnapshot ProcessQueue::drain() {
    m_lock.acquireSharedLock();

    while (m_deque.empty() && !m_closed) {
        SleepConditionVariableSRW(&m_cv, &m_lock.getRawLock(), INFINITE,
                                  CONDITION_VARIABLE_LOCKMODE_SHARED);
    }

    ProcessSnapshot value;
    if (!m_deque.empty()) {
        value = m_deque.front();
    }
    m_lock.releaseSharedLock();
    return value;
}

void ProcessQueue::close() {
    m_lock.acquireExclusiveLock();
    m_closed = true;
    m_lock.releaseExclusiveLock();

    WakeAllConditionVariable(&m_cv);
}
