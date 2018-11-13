#pragma once

#define PRIORITY_SLAM_FASTPATH   16
#define PRIORITY_SLAM_SLOWPATH    8
#define PRIORITY_SLAM_DETECT      6
#define PRIORITY_SLAM_ORB         4
#define PRIORITY_SLAM_RELOCALIZE  2
#define PRIORITY_SLAM_SAVE_MAP    2

#ifdef MYRIAD2
#include "thread_priorities.h"
#define set_priority(p) set_thread_priority(p)
#define get_priority()  get_thread_priority()
#else
#define set_priority(p)
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
    int old = get_priority(), tmp = std::max(old, priority);
    thread_priority avoid_inversion(tmp, old);
    return std::async(policy, [priority, tmp](F &&f, Args &&...args) -> typename std::result_of_t<std::decay_t<F>(typename std::decay_t<Args>...)> {
            if (tmp != priority) set_priority(priority);
            return std::forward<F>(f)(std::forward<Args>(args)...);
        }, std::forward<F>(f), std::forward<Args>(args)...);
}
