#pragma once

#define VOC_FILE 255
#define IDR_VOC_FILE  100

#include <cstddef>

#if defined(_MSC_VER) && (defined(WIN32) || defined(WIN64))
__declspec( dllexport )
#endif
const char *load_vocabulary(size_t &size);
