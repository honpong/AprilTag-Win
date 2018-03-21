/////////////////////////////////////////////////////////////////////
//                                                                 //
// file: BrainStem-all.h                                           //
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

/////////////////////////////////////////////////////////////////////
/// All inclusive Header for C++ API access.

/**
 * This file can be included by users of the C++ API and includes all of
 * the necessary header files which represent the C++ API.
 */

#ifndef __BrainStem_all_H__
#define __BrainStem_all_H__

/////////////////////////////////////////////////////////////////////
/// Core Classes and type definitions for the BrainStem2 library. This
/// includes Link, Module, and the base Entity class.
#ifndef __BrainStem_core_H__
#include "BrainStem-core.h"
#endif //__BrainStem_core_H__

/////////////////////////////////////////////////////////////////////
/// Utility classes for use with the BrainStem library. This
/// currently includes Settings (a command line argument parser).
#ifndef __BrainStem_util_H__
#include "BrainStem-util.h"
#endif //__BrainStem_util_H__

/////////////////////////////////////////////////////////////////////
/// Contains concrete classes for each of the BrainStem Entity Types. This
/// includes System, Store, Analog, etc.
#ifndef __BrainStem_entity_H__
#include "BrainStem-entity.h"
#endif //__BrainStem_entity_H__

#ifndef __a40PinModule_H__
#include "a40PinModule.h"
#endif

#ifndef __aMTMStemModule_H__
#include "aMTMStemModule.h"
#endif

#ifndef __aEtherStem_H__
#include "aEtherStem.h"
#endif

#ifndef __aMTMDAQ1_H__
#include "aMTMDAQ1.h"
#endif

#ifndef __aMTMEtherStem_H__
#include "aMTMEtherStem.h"
#endif

#ifndef __aMTMIOSerial_H__
#include "aMTMIOSerial.h"
#endif

#ifndef __aMTMPM1_H__
#include "aMTMPM1.h"
#endif

#ifndef __aMTMUSBStem_H__
#include "aMTMUSBStem.h"
#endif

#ifndef __aUSBStem_H__
#include "aUSBStem.h"
#endif

#ifndef __aUSBHub2x4_H__
#include "aUSBHub2x4.h"
#endif

#ifndef __aMTMRelay_H__
#include "aMTMRelay.h"
#endif

#ifndef __aUSBHub3p_H__
#include "aUSBHub3p.h"
#endif

#ifndef __aUSBCSwitch_H__
#include "aUSBCSwitch.h"
#endif

#endif //__BrainStem_all_H__
