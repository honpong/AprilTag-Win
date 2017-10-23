#ifndef __D_T_ENDIANNESS__
#define __D_T_ENDIANNESS__

#if defined(__linux__) || defined(__APPLE__)
    #include <sys/param.h>
    #define HOST_IS_LITTLE_ENDIAN (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
    #define HOST_IS_BIG_ENDIAN (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#elif defined(_WIN32) || defined(_WIN64)
    #include <Windows.h>
    #define HOST_IS_LITTLE_ENDIAN (REG_DWORD == REG_DWORD_LITTLE_ENDIAN)
    #define HOST_IS_BIG_ENDIAN (REG_DWORD == REG_DWORD_BIG_ENDIAN)
#elif defined(__myriad2)
    // Programmer's guide 4.2, 2.3.5
    #define HOST_IS_LITTLE_ENDIAN 1
#endif

#if defined(__linux__)  || defined(__myriad2)
    #include <endian.h>
#elif defined(__APPLE__)
    #include <libkern/OSByteOrder.h>
    #define htobe32(x) OSSwapHostToBigInt32(x)
    #define htobe64(x) OSSwapHostToBigInt64(x)
    #define be32toh(x) OSSwapBigToHostInt32(x)
    #define be64toh(x) OSSwapBigToHostInt64(x)
    #define htole32(x) OSSwapHostToLittleInt32(x)
    #define htole64(x) OSSwapHostToLittleInt64(x)
    #define le32toh(x) OSSwapLittleToHostInt32(x)
    #define le64toh(x) OSSwapLittleToHostInt64(x)
#elif defined(_WIN32) || defined(_WIN64)
    #include <stdlib.h>
    #if HOST_IS_BIG_ENDIAN
        #define htobe32(x) (x)
        #define htobe64(x) (x)
        #define be32toh(x) (x)
        #define be64toh(x) (x)
        #define htole32(x) _byteswap_ulong(x)
        #define htole64(x) _byteswap_uint64(x)
        #define le32toh(x) _byteswap_ulong(x)
        #define le64toh(x) _byteswap_uint64(x)
    #elif HOST_IS_LITTLE_ENDIAN
        #define htobe32(x) _byteswap_ulong(x)
        #define htobe64(x) _byteswap_uint64(x)
        #define be32toh(x) _byteswap_ulong(x)
        #define be64toh(x) _byteswap_uint64(x)
        #define htole32(x) (x)
        #define htole64(x) (x)
        #define le32toh(x) (x)
        #define le64toh(x) (x)
    #else
        #error Could not detect endianness on Windows
    #endif
#else
    #error Unknown platform
#endif

#include <algorithm>
#include <cstring>

static bool host_is_little_endian() {
#if HOST_IS_LITTLE_ENDIAN
    return true;
#elif HOST_IS_BIG_ENDIAN
    return false;
#else
    const uint32_t i = 0xffff0000;
    return *reinterpret_cast<const unsigned char*>(&i) == 0;
#endif
}

static void le8toh(const uint8_t* src, int size, uint8_t* dst) {
    if (host_is_little_endian()) {
        memcpy(dst, src, size * sizeof(uint8_t));
    } else {
        std::reverse_copy(src, src + size, dst);
    }
}

#endif  // __D_T_ENDIANNESS__
