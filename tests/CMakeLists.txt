add_executable(WinProcAdminTests
    test_main.cpp
    QueueTests.cpp
    ../src/ProcessQueue.cpp
    ../src/ProcSrwLock.cpp
)
target_link_libraries(WinProcAdminTests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(WinProcAdminTests)
