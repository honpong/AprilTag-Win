/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* file: aMutex.h                                                  */
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

#ifndef _aMutex_H_
#define _aMutex_H_

#ifndef _aDefs_H_
#include "aDefs.h"
#endif // _aDefs_H_

#include "aError.h"

#define aMUTEX_NAME_MAX_LEN    255U

/////////////////////////////////////////////////////////////////////
/// Platform Independent Synchronization Primitive

/** \defgroup aMutex Synchronization Interface
 * \ref aMutex "aMutex.h" Provides a platform independent synchronization
 * mechanism.
 * The link interface and the packe fifos both use this interface for
 * synchronization between threads. Includes facilities for creating,
 * locking and unlocking mutex primitives.
 */



/////////////////////////////////////////////////////////////////////
/// Typedef #aMutexRef Opaque pointer to cross platform Mutex.
typedef void* aMutexRef;

#ifdef __cplusplus
extern "C" {
#endif


    
    /////////////////////////////////////////////////////////////////////
    /// Create a Mutex.
    
    /**
     * Creates a Mutex element and uses the character array as the name
     * of the mutex. 
     * \returns aMutexRef on success or NULL on failure.
     */
    aLIBEXPORT aMutexRef aMutex_Create(const char* name /* can be NULL */);
    
    
    /////////////////////////////////////////////////////////////////////
    /// Mutex Identifier.
    
    /**
     * Gets the character array that represents the mutex' name.
     * \returns A const null terminated character array. This call
     *          does not copy the character array, only presents it for use.
     */
    aLIBEXPORT const char* aMutex_Identifier(aMutexRef mutex);
    
    /////////////////////////////////////////////////////////////////////
    /// Mutex Destroy.
    
    /**
     * Safely destroys a MutexRef, and frees its associated memory.
     * Free should not be called on a MutexRef directly, and all Mutexs 
     * created with aMutex_Create must use aMutex_Destroy to free associated
     * resources properly.
     *
     * \param mutex - Valid MutexRef
     * \returns Function returns aErr values.
     * \retval aErrNone - If the Destruction was successful.
     * \retval aErrParam - If the MutexRef was invalid.
     */
    aLIBEXPORT aErr aMutex_Destroy(aMutexRef* mutex);
    
    
    /////////////////////////////////////////////////////////////////////
    /// Mutex Lock.
    
    /**
     * Blocking attempt to Lock the mutex. The call will not return until,
     * the requesting thread gains control of the mutex, and successfully
     * locks it or some unrecoverable error occured.
     *
     * \returns Function returns aErr values.
     * \retval aErrNone - Successfully aquired the lock.
     * \retval aErrParam - If the MutexRef was invalid.
     * \return aErrDuplicate - If a specific error occured locking the mutex.
     */
    aLIBEXPORT aErr aMutex_Lock(aMutexRef mutex);
    
    /////////////////////////////////////////////////////////////////////
    /// Mutex TryLock.
    
    /**
     * Non Blocking attempt to Lock the mutex. The call will return immediately
     * with aErrBusy if another process or thread owns the lock.
     *
     * \returns Function returns aErr values.
     * \retval aErrNone - Successfully aquired the lock.
     * \retval aErrParam - If the MutexRef was invalid.
     * \retval aErrBusy - If the lock was already in use.
     */
    aLIBEXPORT aErr aMutex_TryLock(aMutexRef mutex);
    
    /////////////////////////////////////////////////////////////////////
    /// Mutex Unlock.
    
    /**
     * Relenquish the lock on the mutex.
     *
     * \returns Function returns aErr values.
     * \retval aErrNone - Successfully unlocked mutex.
     * \retval aErrParam - If the MutexRef was invalid.
     * \retval aErrPermission - If the lock is owned by another thread.
     */
    aLIBEXPORT aErr aMutex_Unlock(aMutexRef mutex);
    
#ifdef __cplusplus
}
#endif

#endif /* _aMutex_H_ */
