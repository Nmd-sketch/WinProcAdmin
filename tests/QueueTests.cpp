#include <gtest/gtest.h>
#include "../include/ProcessQueue.hpp"
#include <thread>


TEST(Queue, NoDataRacesDuringReadWrite) {
    ProcessQueue pQueue{5};
    unsigned int writeCounter = 0;
    unsigned int readCounter = 0;

    std::thread t1{[&pQueue, &readCounter]() {
       while (readCounter <= 100) {
            pQueue.try_pop();
            readCounter++;
        }    
    }};

    std::thread t2{[&pQueue, &writeCounter]() {
        for (; writeCounter <= 100; writeCounter++) {
            ProcessSnapshot p = {{"Proc", writeCounter, writeCounter + 1}};
            pQueue.push(std::move(p));
        }
        pQueue.close();
    }};

    t2.join();
    t1.join();

    ASSERT_EQ(readCounter, writeCounter);
}
