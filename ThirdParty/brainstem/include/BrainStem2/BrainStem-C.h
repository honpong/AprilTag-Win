/////////////////////////////////////////////////////////////////////
//                                                                 //
// file: BrainStem-C.h                                             //
//                                                                 //
/////////////////////////////////////////////////////////////////////
//                                                                 //
// description: BrainStem API's and support.                       //
//                                                                 //
//                                                                 //
/////////////////////////////////////////////////////////////////////
//                                                                 //
// Copyright 1994-2015. Acroname Inc.                              //
//                                                                 //
// This software is the property of Acroname Inc.  Any             //
// distribution, sale, transmission, or re-use of this code is     //
// strictly forbidden except with permission from Acroname Inc.    //
//                                                                 //
// To the full extent allowed by law, Acroname Inc. also excludes  //
// for itself and its suppliers any liability, whether based in    //
// contract or tort (including negligence), for direct,            //
// incidental, consequential, indirect, special, or punitive       //
// damages of any kind, or for loss of revenue or profits, loss of //
// business, loss of information or data, or other financial loss  //
// arising out of or in connection with this software, even if     //
// Acroname Inc. has been advised of the possibility of such       //
// damages.                                                        //
//                                                                 //
// Acroname Inc.                                                   //
// www.acroname.com                                                //
// 720-564-0373                                                    //
//                                                                 //
/////////////////////////////////////////////////////////////////////

#ifndef _BrainStem2_H_
#define _BrainStem2_H_

/////////////////////////////////////////////////////////////////////
/// All inclusive Header for C-API access.

/**
 * This file can be included by users of the C-API and includes all of
 * the necessary header files which represent the C-API.
 */

/////////////////////////////////////////////////////////////////////
/// Cross platform defines and system includes.
#ifndef _aDefs_H_
#include "aDefs.h"
#endif

/////////////////////////////////////////////////////////////////////
/// BrainStem discovery functions for USB and TCPIP based stems.
#ifndef _aDiscover_H_
#include "aDiscover.h"
#endif

/////////////////////////////////////////////////////////////////////
/// BrainStem Error Codes.
#ifndef _aError_H_
#include "aError.h"
#endif

/////////////////////////////////////////////////////////////////////
/// Filesystem interaction, CRUD access.
#ifndef _aFile_H_
#include "aFile.h"
#endif

/////////////////////////////////////////////////////////////////////
/// Link to BrainStem.
#ifndef _aLink_H_
#include "aLink.h"
#endif

/////////////////////////////////////////////////////////////////////
/// Thread synchronization primitive.
#ifndef _aMutex_H_
#include "aMutex.h"
#endif

/////////////////////////////////////////////////////////////////////
/// Packet structure and accessors.
#ifndef _aPacket_H_
#include "aPacket.h"
#endif

/////////////////////////////////////////////////////////////////////
/// Stream Access, Read and Write.
#ifndef _aStream_H_
#include "aStream.h"
#endif

/////////////////////////////////////////////////////////////////////
/// Sleep and Timer functions.
#ifndef _aTime_H_
#include "aTime.h"
#endif

/////////////////////////////////////////////////////////////////////
/// UEI functions.
#ifndef _aUEI_H_
#include "aUEI.h"
#endif

/////////////////////////////////////////////////////////////////////
/// Sleep and Timer functions.
#ifndef _aVersion_H_
#include "aVersion.h"
#endif

#endif //_BrainStem2_H_
