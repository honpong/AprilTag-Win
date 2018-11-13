#include <pthread.h>
#include <sched.h>
#include <assert.h>

static inline void set_thread_priority(int priority)
{
    struct sched_param param;
    int e, policy;

    e = pthread_getschedparam(pthread_self(), &policy, &param);
    assert(e == 0);
    if (e) return;

    param.sched_priority = priority;

    assert(sched_get_priority_min(policy) <= param.sched_priority && param.sched_priority <= sched_get_priority_max(policy));

    e = pthread_setschedparam(pthread_self(), policy, &param);
    assert(e == 0);
    if (e) return;
}
