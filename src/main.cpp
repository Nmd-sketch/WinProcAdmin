#include <iostream>
#include <queue>
#include <thread>
#include "../include/gui/gui.hpp"
#include "../include/ProcessMonitor.hpp"
int main()
{
    ProcessQueue pQueue{10};
    ProcessMonitor pMon{1000};
    Gui app{640, 480};
    app.setProcessQueue(&pQueue);
    pMon.setProcessQueue(&pQueue);
    std::thread t1{[&app, &pMon](){app.startGui(); pMon.stopPolling();}};
    pMon.startPolling();
    t1.join();
    return 0;
}
