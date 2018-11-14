#pragma once

#define PRIORITY_SLAM_FASTPATH   16
#define PRIORITY_SLAM_SLOWPATH    8
#define PRIORITY_SLAM_DETECT     18
#define PRIORITY_SLAM_ORB        17
#define PRIORITY_SLAM_DBOW        6
#define PRIORITY_SLAM_RELOCALIZE  2
#define PRIORITY_SLAM_SAVE_MAP    2

#ifdef MYRIAD2
#include <pthread.h>
#include <sched.h>
#include <assert.h>
inline int get_priority()
{
    struct sched_param param; int policy;
    return pthread_getschedparam(pthread_self(), &policy, &param) == 0 ? param.sched_priority : 0;
}

inline void set_priority(int priority)
{
    struct sched_param param;
    int policy, e = pthread_getschedparam(pthread_self(), &policy, &param);
    assert(e == 0);
    if (e) return;

    param.sched_priority = priority;

    assert(sched_get_priority_min(policy) <= param.sched_priority && param.sched_priority <= sched_get_priority_max(policy));

    e = pthread_setschedparam(pthread_self(), policy, &param);
    assert(e == 0);
    if (e) return;
}
#else
inline void set_priority(int) { }
inline int get_priority() { return 0; }
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
