#include "gtest/gtest.h"
#include "debug_log.h"

TEST(DebugLog, Set)
{
    int counter = 0;
    debug_log_set([&counter](void * handle, int level, const char * message, size_t message_size) {
            counter++;
            },
            3, nullptr);
    debug_log->info("test");
    EXPECT_EQ(counter, 1);
    debug_log->info("test");
    debug_log->info("test");
    EXPECT_EQ(counter, 3);
}

std::atomic<int> thread_counter;

void log_ten_times()
{
    for(int i = 0; i < 10; i++)
        debug_log->info("test");
}

void set_log()
{
    debug_log_set([](void * handle, int level, const char * message, size_t message_size) {
        thread_counter++;
    }, 3, nullptr);
}

#include <thread>
TEST(DebugLog, Threads)
{

    thread_counter = 0;
    set_log();
    std::vector<std::thread> workers;
    int n_threads = 50;
    for(int i = 0; i < n_threads; i++)
        workers.push_back(std::thread(log_ten_times));

    for(auto &t : workers)
        t.join();
    EXPECT_EQ(thread_counter, 10*n_threads);
}

TEST(DebugLog, ThreadSet)
{

    std::vector<std::thread> workers;
    int n_threads = 50;
    for(int i = 0; i < n_threads; i++)
        workers.push_back(std::thread(set_log));

    for(auto &t : workers)
        t.join();
}
