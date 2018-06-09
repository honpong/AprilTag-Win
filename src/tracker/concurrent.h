#ifndef __CONCURRENT_H
#define __CONCURRENT_H

#include <mutex>
#include <thread>

// Replacement for std::lock on TM2 when no round robin scheduling
template<typename ... T>
class multiple_lock_guard {
 public:
    static_assert(2 <= sizeof...(T) && sizeof...(T) <= 3,
                  "multiple_lock_guard supports between 2 and 3 objects");
    multiple_lock_guard(const multiple_lock_guard&) = delete;
    multiple_lock_guard(multiple_lock_guard&&) = delete;
    multiple_lock_guard(T&... m) : m_(std::forward_as_tuple(m...)) { lock(); }
    ~multiple_lock_guard() { unlock(); }
 private:
    std::tuple<T&...> m_;

    template<int N = sizeof...(T)>
    typename std::enable_if<N == 2, void>::type
    lock() {
        auto& m0 = std::get<0>(m_);
        auto& m1 = std::get<1>(m_);
        if ((void*)&m0 < (void*)&m1) {
            m0.lock(); m1.lock();
        } else {
            m1.lock(); m0.lock();
        }
    }

    template<int N = sizeof...(T)>
    typename std::enable_if<N == 2, void>::type
    unlock() {
        auto& m0 = std::get<0>(m_);
        auto& m1 = std::get<1>(m_);
        if ((void*)&m0 < (void*)&m1) {
            m1.unlock(); m0.unlock();
        } else {
            m0.unlock(); m1.unlock();
        }
    }

    template<int N = sizeof...(T)>
    typename std::enable_if<N == 3, void>::type
    lock() {
        auto& m0 = std::get<0>(m_);
        auto& m1 = std::get<1>(m_);
        auto& m2 = std::get<2>(m_);
        if ((void*)&m0 < (void*)&m1) {
            if ((void*)&m1 < (void*)&m2) {
                m0.lock(); m1.lock(); m2.lock();
            } else if ((void*)&m0 < (void*)&m2) {
                m0.lock(); m2.lock(); m1.lock();
            } else {
                m2.lock(); m0.lock(); m1.lock();
            }
        } else {
            if ((void*)&m0 < (void*)&m2) {
                m1.lock(); m0.lock(); m2.lock();
            } else if ((void*)&m1 < (void*)&m2) {
                m1.lock(); m2.lock(); m0.lock();
            } else {
                m2.lock(); m1.lock(); m0.lock();
            }
        }
    }

    template<int N = sizeof...(T)>
    typename std::enable_if<N == 3, void>::type
    unlock() {
        auto& m0 = std::get<0>(m_);
        auto& m1 = std::get<1>(m_);
        auto& m2 = std::get<2>(m_);
        if ((void*)&m0 < (void*)&m1) {
            if ((void*)&m1 < (void*)&m2) {
                m2.unlock(); m1.unlock(); m0.unlock();
            } else if ((void*)&m0 < (void*)&m2) {
                m1.unlock(); m2.unlock(); m0.unlock();
            } else {
                m1.unlock(); m0.unlock(); m2.unlock();
            }
        } else {
            if ((void*)&m0 < (void*)&m2) {
                m2.unlock(); m0.unlock(); m1.unlock();
            } else if ((void*)&m1 < (void*)&m2) {
                m0.unlock(); m2.unlock(); m1.unlock();
            } else {
                m0.unlock(); m1.unlock(); m2.unlock();
            }
        }
    }
};

template<typename T0, typename Fun, typename... Args>
typename std::result_of<typename std::decay<Fun>::type(typename std::decay<Args>::type...)>::type
critical_section(T0& m0, Fun &&fun, Args &&...args) {
    std::lock_guard<T0> lock0(m0);
    return std::forward<Fun>(fun)(std::forward<Args>(args)...);
}

template<typename T0, typename T1, typename Fun, typename... Args>
typename std::result_of<typename std::decay<Fun>::type(typename std::decay<Args>::type...)>::type
critical_section(T0& m0, T1& m1, Fun &&fun, Args &&...args) {
    multiple_lock_guard<T0, T1> lock(m0, m1);
    return std::forward<Fun>(fun)(std::forward<Args>(args)...);
}

template<typename T0, typename T1, typename T2, typename Fun, typename... Args>
typename std::result_of<typename std::decay<Fun>::type(typename std::decay<Args>::type...)>::type
critical_section(T0& m0, T1& m1, T2& m2, Fun &&fun, Args &&...args) {
    multiple_lock_guard<T0, T1, T2> lock(m0, m1, m2);
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
