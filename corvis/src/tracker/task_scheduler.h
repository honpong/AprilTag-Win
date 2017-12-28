#ifndef __TASK_SCHEDULER_H
#define __TASK_SCHEDULER_H

#include <future>

namespace scheduler {

/** Scheduling policies determine when a function given to
 * task_scheduler::process is executed.
 *
 * RUN_EVERY_N_CALLS: only one function in N calls to "process" is run.
 *   N is configurable. This policy produces deterministic results always.
 *
 * RUN_WHEN_POSSIBLE: run the function if the background thread is free.
 *   If threading mode is not activated, the function is always executed
 *   serially. This policy produces undeterministic results when threaded.
 */
enum policy {
    RUN_EVERY_N_CALLS,
    RUN_WHEN_POSSIBLE
};

/** Status of the return value of task_scheduler::process.
 *
 * RESULT_AVAILABLE: the current call or a previous call to "process" has
 *   produced a result.
 *
 * RESULT_UNAVAILABLE: there is not any result available yet.
 */
enum result_status {
    RESULT_AVAILABLE,
    RESULT_UNAVAILABLE
};

}  // namespace scheduler

template<typename T>
class task_scheduler {
 public:
    struct result {
        scheduler::result_status status;
        T value;
    };

 public:
    /** Creates the scheduler with a given scheduling policy.
     * @see set_policy
     */
    task_scheduler(scheduler::policy policy = scheduler::RUN_EVERY_N_CALLS,
                   int run_every_n_calls = 1);

    /** Sets the scheduling policy.
     * @param policy policy
     * @param call_interval interval of calls to run a function when
     * policy == RUN_EVERY_N_CALLS.
     */
    void set_policy(scheduler::policy policy, int call_interval = 1);

    /** Returns the current policy.
     */
    scheduler::policy get_policy() const;

    /** Returns the interval of calls when the policy is RUN_EVERY_N_CALLS.
     * Undefined otherwise.
     */
    int get_call_interval() const;

    /** Processes a function.
     * The function is executed depending on the current scheduling policy:
     * RUN_EVERY_N_CALLS: only one in N calls to "process" is executed. The
     * function given to the first call is always executed.
     * RUN_WHEN_POSSIBLE: if threaded, the function is executed if the
     * background thread is not busy. If not threaded, the function is always
     * executed.
     * If the policy does not allow it, the function is not executed, i.e.
     * it is ignored and no side effect take place.
     *
     * @param threaded if true, the function is run on a background thread.
     * @param fun function to be executed. The function may be executed now,
     * in the future, or no executed at all.
     * @param args optional arguments to "fun".
     * @return if threaded == false, "process" returns current "fun"'s returning
     * value and the status is RESULT_AVAILABLE always.
     * If threaded == true, "process" may or may not return the result of a
     * previously executed function.
     * @note if threaded == false but "process" was previously called with
     * threaded == true, the return value of the old "fun" function is ignored.
     * @note if a previous call to "process" was done with threaded == true,
     * and the current "fun" is going to be executed, the calling thread
     * is blocked until the background thread finishes the execution of any
     * old "fun".
     */
    template<typename Fun, typename... Args>
    result process(bool threaded, const Fun& fun, Args... args);

    /** Returns the number of calls to "process" in which the functions
     * were missed and not executed.
     */
    size_t get_missing_calls() const;

    /** Returns the number of calls to "process".
     */
    size_t get_total_calls() const;

 private:
   struct criteria {
       scheduler::policy policy;
       struct {
           int current_call_index;
           int call_interval;
       } every_n;
   };

   struct stats {
       size_t missing_calls;
       size_t total_calls;
   };

   criteria criteria_;
   std::future<T> future_;
   stats stats_;
};

template<typename T>
task_scheduler<T>::task_scheduler(scheduler::policy policy,
                                  int run_every_n_calls) {
    set_policy(policy, run_every_n_calls);
    stats_.missing_calls = 0;
    stats_.total_calls = 0;
}

template<typename T>
void task_scheduler<T>::set_policy(scheduler::policy policy,
                                   int run_every_n_calls) {
    criteria_.policy = policy;
    criteria_.every_n.call_interval = std::max(1, run_every_n_calls);
    criteria_.every_n.current_call_index = 0;
}

template<typename T>
scheduler::policy task_scheduler<T>::get_policy() const {
    return criteria_.policy;
}

template<typename T>
int task_scheduler<T>::get_call_interval() const {
    return criteria_.every_n.call_interval;
}

template<typename T>
size_t task_scheduler<T>::get_missing_calls() const {
    return stats_.missing_calls;
}

template<typename T>
size_t task_scheduler<T>::get_total_calls() const {
    return stats_.total_calls;
}

template<typename T>
template<typename Fun, typename... Args>
typename task_scheduler<T>::result task_scheduler<T>::process(bool threaded,
                                                              const Fun& fun,
                                                              Args... args) {
    static_assert(std::is_same<T, typename std::result_of<Fun&&(Args&&...)>::type>::value,
                  "Function's return type and task_scheduler's type do not match");

    bool run_now;
    switch (criteria_.policy) {
    case scheduler::policy::RUN_EVERY_N_CALLS:
        run_now = (criteria_.every_n.current_call_index == 0);
        criteria_.every_n.current_call_index =
                (criteria_.every_n.current_call_index + 1) %
                criteria_.every_n.call_interval;
        break;

    case scheduler::policy::RUN_WHEN_POSSIBLE:
        if (threaded && future_.valid()) {
            auto status = future_.wait_for(std::chrono::milliseconds(0));
            run_now = (status == std::future_status::ready);
        } else {
            run_now = true;
        }
        break;
    }

    result ret;
    ret.status = scheduler::result_status::RESULT_UNAVAILABLE;
    if (run_now) {
        if (threaded) {
            if (future_.valid()) {
                ret.status = scheduler::result_status::RESULT_AVAILABLE;
                ret.value = future_.get();
            }
            future_ = std::async(std::launch::async,
                                 fun, std::forward<Args>(args)...);
        } else {
            if (future_.valid()) future_.get();
            ret.status = scheduler::result_status::RESULT_AVAILABLE;
            ret.value = fun(std::forward<Args>(args)...);
        }
    } else {
        ++stats_.missing_calls;
    }
    ++stats_.total_calls;
    return ret;
}

#endif  // __TASK_SCHEDULER_H
