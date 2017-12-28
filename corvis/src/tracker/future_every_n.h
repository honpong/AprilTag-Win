#ifndef __FUTURE_EVERY_N_H
#define __FUTURE_EVERY_N_H

#include <future>

/** Auxiliary class to encode a future<T> that must be executed every N times.
 * @tparam T T type of future<T>
 * @tparam N the future must be set 1 in every N times.
 */
template<typename T, unsigned int N = 1>
class future_every_n : public std::future<T> {
 public:
    /** Returns true iff the shared state has been valid for N calls to valid_n.
     */
    bool valid_n() {
        if (std::future<T>::valid()) {
            bool yes = (++index_ >= N);
            if (yes) index_ = 0;
            return yes;
        } else {
            return false;
        }
    }

    future_every_n() : index_(0) {}
    future_every_n(future_every_n&) = delete;
    future_every_n(future_every_n&& rhs) : std::future<T>(std::move(rhs)) {
        index_ = rhs.index_;
        rhs.index_ = 0;
    }
    future_every_n<T>& operator=(future_every_n&& rhs) {
        std::future<T>::operator=(std::move(rhs));
        index_ = rhs.index_;
        rhs.index_ = 0;
        return *this;
    }
    future_every_n(std::future<T>&& rhs) : std::future<T>(std::move(rhs)), index_(0) {}
    future_every_n<T>& operator=(std::future<T>&& rhs) {
        std::future<T>::operator=(std::move(rhs));
        index_ = 0;
        return *this;
    }
 private:
    unsigned int index_;
};

#endif  // __FUTURE_EVERY_N_H
