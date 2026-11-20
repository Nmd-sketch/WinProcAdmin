#pragma once
#include "ProcSrwLock.hpp"
#include <deque>
#include <vector>
#include "ProcessData.hpp"


class ProcessQueue {
private:
    ProcSrwLock m_lock;
    std::deque<ProcessSnapshot> m_deque;
    bool  m_closed;
    CONDITION_VARIABLE m_cv;
    ssize_t max_size;

public:
    explicit ProcessQueue(ssize_t maxDequeSize);

    void push(ProcessSnapshot snap) noexcept;
    bool try_pop();
    ProcessSnapshot drain();
    void close();
};
