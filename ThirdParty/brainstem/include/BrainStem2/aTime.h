/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* file: aTime.h                                                   */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* description: Definition of platform-independent time utils	   */
/*                                                                 */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* Copyright 1994-2015. Acroname Inc.                              */
/*                                                                 */
/* This software is the property of Acroname Inc.  Any             */
/* distribution, sale, transmission, or re-use of this code is     */
/* strictly forbidden except with permission from Acroname Inc.    */
/*                                                                 */
/* To the full extent allowed by law, Acroname Inc. also excludes  */
/* for itself and its suppliers any liability, whether based in    */
/* contract or tort (including negligence), for direct,            */
/* incidental, consequential, indirect, special, or punitive       */
/* damages of any kind, or for loss of revenue or profits, loss of */
/* business, loss of information or data, or other financial loss  */
/* arising out of or in connection with this software, even if     */
/* Acroname Inc. has been advised of the possibility of such       */
/* damages.                                                        */
/*                                                                 */
/* Acroname Inc.                                                   */
/* www.acroname.com                                                */
/* 720-564-0373                                                    */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef _aTime_H_
#define _aTime_H_

#ifndef _aDefs_H_
#include "aDefs.h"
#endif // _aDefs_H_

#include "aError.h"

/////////////////////////////////////////////////////////////////////
/// Basic Time procedures Sleep and Get process tics.

/** \defgroup aTime Time Interface
 * \ref aTime "aTime.h" provides a platform independent interface for
 * millisecond sleep, and for getting process tics.
 */

#ifdef __cplusplus
extern "C" {
#endif
    
    /////////////////////////////////////////////////////////////////////
    /// Get the current tick count in milliseconds.
    
    /**
     * This call returns a number of milliseconds. Depending on the platform,
	 * this can be the number of milliseconds since the last boot, or from the
	 * epoc start. As such, this call should not be used as an external reference
	 * clock. It is accurate when used as a differential, i.e. internal, measurement only.
     *
     * \return unsigned long number of milliseconds elapsed. 
     */
    aLIBEXPORT unsigned long aTime_GetMSTicks(void);
    
    
    /////////////////////////////////////////////////////////////////////
    /// Sleep the current process for msTime milliseconds.
    
    /**
     * Sleeps the current process. This is not an active sleep, there are no
     * signals which will "wake" the process.
     *
     * \param msTime Milliseconds to sleep.
     *
     * \returns Function returns aErr values.
     * \retval aErrNone The call returned successfully.
     * \retval aErrUnknown Um unknown what went wrong.
     */
    aLIBEXPORT aErr aTime_MSSleep(const unsigned long msTime);
    
#ifdef __cplusplus
}
#endif

#endif /* _aTime_H_ */
