///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @defgroup MemLogger    Memory Logger
/// @defgroup dbgMemLoggerApi Memory Logger Utility
/// @ingroup  MemLogger
/// @{
/// @brief Header with Memory Logger API prototypes
///
/// This is the API for the tracer and memory Logger utilities
///

#ifndef DBG_MEM_LOGGER_API_H
#define DBG_MEM_LOGGER_API_H

#include <mv_types.h>
#include <dbgLogEvents.h>
#include <dbgMemLoggerApiDefines.h>

#ifdef __cplusplus
extern "C" {
#endif

/// @brief Logs memory events by filling the dedicated buffer.
/// 
/// The buffer is used as ring buffer.
/// This function is passed as a parameter to the Init dbg API function of the Tracer.
/// @param[in] eventId - Event ID
/// @param[in] data    - Data field containing custom info
/// @return void
///
void dbgMemLogEvent(Event_t eventId, u32 data);


/// @brief Attach the user provided buffer to the pointer used by the Mem Logger
/// @param[in] memPtr - pointer to the user provided buffer
/// @param[in] size   - the size of the above buffer
/// @return void
///
void dbgMemLogInit(u8* memPtr, u32 size);

#ifdef __cplusplus
}
#endif


#endif //_DBG_MEM_LOGGER_API_H_
