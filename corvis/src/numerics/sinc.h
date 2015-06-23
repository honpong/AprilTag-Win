#ifndef __SINC_H__
#define __SINC_H__

#include <cmath>
#include <limits>

// sin(x)/x
template <typename f_t> static inline f_t sinc(f_t x, f_t x2) {
    return x2 < std::sqrt(f_t(120) * std::numeric_limits<f_t>::epsilon()) ? f_t(1) - f_t(1)/f_t(6) * x2 : std::sin(x) / x;
}
template <typename f_t> static inline f_t sinc(f_t x) {
    return sinc(x, x*x);
}

// (1-cos(x))/x^2
template <typename f_t> static inline f_t cosc(f_t x, f_t x2) {
    return x2 < std::sqrt(f_t(720) * std::numeric_limits<f_t>::epsilon()) ? f_t(1)/f_t(2) - f_t(1)/f_t(24) * x2  : (f_t(1)-std::cos(x)) / x2;
}
template <typename f_t> static inline f_t cosc(f_t x) {
    return cosc(x, x*x);
}

// (x-sin(x))/x^3
template <typename f_t> static inline f_t sincc(f_t x, f_t x2) {
    return x2 < std::sqrt(f_t(5040) * std::numeric_limits<f_t>::epsilon()) ? f_t(1)/f_t(6) - f_t(1)/f_t(120) * x2 : (x-std::sin(x)) / (x*x2);
}
template <typename f_t> static inline f_t sincc(f_t x) {
    return sincc(x, x*x);
}

#endif//__SINC_H__
