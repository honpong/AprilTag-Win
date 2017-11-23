#pragma once

#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

template<typename T>
T popcount(T v) {
#if defined(__GNUC__) || __has_builtin(__builtin_popcountll)
    if (std::is_same<unsigned int, T>::value)
        return __builtin_popcount(v);
    else if (std::is_same<unsigned long, T>::value)
        return __builtin_popcountl(v);
    else if (std::is_same<unsigned long long, T>::value)
        return __builtin_popcountll(v);
#elif defined(_WIN64)
    if (std::is_same<unsigned short, T>::value)
        return __popcnt16(v);
    else if (std::is_same<unsigned int, T>::value)
        return __popcnt(v);
    else if (std::is_same<unsigned __int64, T>::value)
        return __popcnt64(v);
#endif
    // taken from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
    v = v - ((v >> 1) & (T)~(T)0/3);
    v = (v & (T)~(T)0/15*3) + ((v >> 2) & (T)~(T)0/15*3);
    v = (v + (v >> 4)) & (T)~(T)0/255*15;
    return (T)(v * ((T)~(T)0/255)) >> (sizeof(T) - 1) * CHAR_BIT;
}
