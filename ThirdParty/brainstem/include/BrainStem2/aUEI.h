/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* file: aUEI.h                                                    */
/*                                                                 */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                 */
/* description: UEI processing utilities.                          */
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

#ifndef _aUEI_H_
#define _aUEI_H_

#ifndef _aDefs_H_
#include "aDefs.h"
#endif

/////////////////////////////////////////////////////////////////////
/// UEI Utilities.

/** \defgroup aUEI UEI Utilities
 * \ref aUEI "aUEI.h" Provides structs and utilities for working with UEIs.
 */

// Mark: -UEI data struct

/// Typedef Enum #dataType 

/**
 * UEI datatype
 */
typedef enum {
    aUEI_VOID = 0, ///< Void datatype.
    aUEI_BYTE = 1, ///< Char datatype.
    aUEI_SHORT = 2, ///< Short datatype.
    aUEI_INT = 4, ///< Int datatype.
} dataType;

/// Typedef Struct #uei

/**
 * UEI data struct.
 */
typedef struct {
    uint8_t module; ///< Module address.
    uint8_t command; ///< Command code.
    uint8_t option; ///< option code & UEI operation.
    uint8_t specifier; ///< Entity index & response specifier.
    union { 
        uint8_t byteVal; ///< Char value union member.
        uint16_t shortVal; ///< Short value union member.
        uint32_t intVal; ///< Int value union member.
    } v;
    
    dataType type; ///< Union dataType.
} uei;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

    /// Retreive a short from a UEI.

    /**
     * \param p Pointer to byte array containing short.
     * \returns uint16_t The short value.
     */
    aLIBEXPORT uint16_t aUEI_RetrieveShort(const uint8_t* p);

    /// Store a short in a UEI.

    /**
     * \param p Pointer to uei shortVal.
     * \param v Short value to store.
     */
    void aUEI_StoreShort(uint8_t* p, uint16_t v);
    
    /// Retreive an Int from a UEI.

    /**
     * \param p Pointer to byte array containing the Int.
     * \returns uint32_t The integer value.
     */
    aLIBEXPORT uint32_t aUEI_RetrieveInt(const uint8_t* p);
    
    /// Store an Int in a UEI.

    /** 
     * \param p Pointer to the IntVal of a UEI.
     * \param v The value to store.
     */
    void aUEI_StoreInt(uint8_t* p, uint32_t v);
    
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif // _aReflex_H_
