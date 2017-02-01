#pragma once

#include "vec4.h"

template <int N>
class stdev
{
public:
    v<N> sum, min, max, mean, M2, variance, stdev_;
    uint32_t count;
    stdev(): sum(v<N>::Zero()), min(1/v<N>::Zero().array()), max(v<N>::Zero()), mean(v<N>::Zero()), M2(v<N>::Zero()), variance(v<N>::Zero()), stdev_(v<N>::Zero()), count(0) {}
    bool valid() { return count >= 2; }
    void data(const v<N> &x) {
        ++count;
        v<N> delta = x - mean;
        mean = mean + delta / (f_t)count;
        M2 = M2 + delta.cwiseProduct(x - mean);
        if(x.norm() > max.norm()) max = x;
        if(x.norm() < min.norm()) min = x;
        variance = M2 / (f_t)(count - 1);
        stdev_ = variance.array().sqrt();
    }
    template <typename Stream>
    friend Stream& operator<<(Stream &stream, const stdev<N> &v) {
        return stream << " min is " << v.min << " mean is: " << v.mean << ", stdev is: " << v.stdev_ << ", max is: " << v.max;
    }
};

