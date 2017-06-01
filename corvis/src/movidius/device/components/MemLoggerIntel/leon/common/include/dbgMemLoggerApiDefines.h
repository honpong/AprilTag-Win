///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @defgroup dbgMemLoggerApiDefines Memory Logger API Defines
/// @ingroup  MemLogger
/// @{
/// @brief Header with types and other definitions for Mem Logger
///
/// This file contains all the definitions of constants, typedefs,
/// structures, enums and exported variables for the Mem Logger component
///

#ifndef DBG_MEM_LOGGER_API_DEFINES_H
#define DBG_MEM_LOGGER_API_DEFINES_H

#include "mv_types.h"

/// @brief Data structure of a Trace Buffer element
typedef struct 
{
	u32 time;
	u32 id;
	u32 data;	
} Node_t;

#ifndef TRACE_BUFFER_SIZES
#define TRACE_BUFFER_SIZES (1024)
#endif

/// @}
#endif //_DBG_MEM_LOGGER_API_DEFINES_H_

