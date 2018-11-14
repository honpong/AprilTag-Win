#pragma once

#define PRIORITY_SLAM_FASTPATH   16
#define PRIORITY_SLAM_SLOWPATH    8
#define PRIORITY_SLAM_DETECT     18
#define PRIORITY_SLAM_ORB        17
#define PRIORITY_SLAM_DBOW        6
#define PRIORITY_SLAM_RELOCALIZE  2
#define PRIORITY_SLAM_SAVE_MAP    2

#ifdef MYRIAD2
#include "thread_priorities.h"
#define set_priority(p) set_thread_priority(p)
#define get_priority()  get_thread_priority()
#else
#define set_priority(p) (void)(p)
#define get_priority() 0
#endif

#include <future>

struct thread_priority {
    int old; bool set;
    thread_priority(int priority, int old_ = get_priority()) : old(old_), set(old != priority) {
        if (set) set_priority(priority);
    }
    ~thread_priority() {
        if (set) set_priority(old);
    }
};

template <typename F, typename... Args>
std::future<std::result_of_t<std::decay_t<F>(std::decay_t<Args>...)>> async_w_priority(int priority, std::launch policy, F &&f, Args &&...args) {
    int old = get_priority();
    thread_priority avoid_inversion(std::max(priority, old), old);
    return std::async(policy, [priority](F &&f, Args &&...args) -> typename std::result_of_t<std::decay_t<F>(typename std::decay_t<Args>...)> {
            set_priority(priority);
            return std::forward<F>(f)(std::forward<Args>(args)...);
        }, std::forward<F>(f), std::forward<Args>(args)...);
}
