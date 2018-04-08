#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Note this uses clock cycle counts and will be different on LOS and
// LRT
uint64_t TimestampNs();

#ifdef __cplusplus
}
#endif
