#ifndef __CONCURRENT_H
#define __CONCURRENT_H

#include <mutex>
#include <thread>

template<typename T0, typename T1, typename ... T2>
void multiple_lock(int i, T0& m0, T1& m1, T2& ...m2) {
    // mdk std::lock may hang on TM2 when no round robin scheduling
    while (true) {
        switch(i) {
        case 0:
            i = std::try_lock(m0, m1, m2...);
            break;
        case 1:
            i = std::try_lock(m1, m2..., m0);
            break;
        default:
            multiple_lock(i - 2, m2..., m0, m1);
            return;
        }
        if (i == -1) return;
        i = (i == sizeof...(m2) + 1 ? 0 : i + 1);
        std::this_thread::yield();
    }
}

template<typename T0, typename T1, typename ... T2>
void multiple_lock(T0& m0, T1& m1, T2& ...m2) {
    multiple_lock(0, m0, m1, m2...);
}

template<typename T0, typename Fun, typename... Args>
typename std::result_of<typename std::decay<Fun>::type(typename std::decay<Args>::type...)>::type
critical_section(T0& m0, Fun &&fun, Args &&...args) {
    std::lock_guard<T0> lock0(m0);
    return std::forward<Fun>(fun)(std::forward<Args>(args)...);
}

template<typename T0, typename T1, typename Fun, typename... Args>
typename std::result_of<typename std::decay<Fun>::type(typename std::decay<Args>::type...)>::type
critical_section(T0& m0, T1& m1, Fun &&fun, Args &&...args) {
    multiple_lock(0, m0, m1);
    std::lock_guard<T0> lock0(m0, std::adopt_lock);
    std::lock_guard<T1> lock1(m1, std::adopt_lock);
    return std::forward<Fun>(fun)(std::forward<Args>(args)...);
}

template<typename T0, typename T1, typename T2, typename Fun, typename... Args>
typename std::result_of<typename std::decay<Fun>::type(typename std::decay<Args>::type...)>::type
critical_section(T0& m0, T1& m1, T2& m2, Fun &&fun, Args &&...args) {
    multiple_lock(0, m0, m1, m2);
    std::lock_guard<T0> lock0(m0, std::adopt_lock);
    std::lock_guard<T1> lock1(m1, std::adopt_lock);
    std::lock_guard<T2> lock2(m2, std::adopt_lock);
    return std::forward<Fun>(fun)(std::forward<Args>(args)...);
}

/** Auxiliary class to represent data shared among threads and protected
 * with a mutex.
 */
template<typename T, typename M = std::mutex>
class concurrent {
 public:
    typedef T value_type;

    /** Runs an arbitrary function after acquiring the mutex.
     * The given function can safely use the concurrent data.
     * The calling thread is blocked if necessary.
     */
    template<typename Fun, typename... Args>
    typename std::result_of<typename std::decay<Fun>::type(typename std::decay<Args>::type...)>::type
    critical_section(Fun &&fun, Args &&...args) const {
        std::lock_guard<M> lock(mutex_);
        return std::forward<Fun>(fun)(std::forward<Args>(args)...);
    }

    template<typename Result, typename Obj, typename ...MArgs, typename ...Args>
    Result
    critical_section(Result (Obj::*m)(MArgs...), Obj *obj, Args &&...args) const {
        std::lock_guard<M> lock(mutex_);
        return (obj->*m)(std::forward<Args>(args)...);
    }

    /** Access to data. Unsafe unless used inside critical_section.
     */
    T* operator->() { return &object_; }
    const T* operator->() const { return &object_; }
    T& operator*() { return object_; }
    const T& operator*() const { return object_; }

    /* Returns the mutex.
     */
    M& mutex() const { return mutex_; }

    /** Implements Lockable interface
     */
    void lock() const { mutex_.lock(); }
    void unlock() const { mutex_.unlock(); }
    bool try_lock() const { return mutex_.try_lock(); }

 private:
    T object_;
    mutable M mutex_;
};

#endif  // __CONCURRENT_H
