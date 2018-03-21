/////////////////////////////////////////////////////////////////////
//                                                                 //
// file: aEtherStem.h                                              //
//                                                                 //
/////////////////////////////////////////////////////////////////////
//                                                                 //
// description: Definition of the Acroname 40-pin module object.   //
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

#ifndef __aEtherStem_H__
#define __aEtherStem_H__

#include "BrainStem-all.h"
#include "aProtocoldefs.h"

#define aETHERSTEM_MODULE_ADDRESS                   a40PINSTEM_MODULE

#define aETHERSTEM_NUM_STORES                   a40PINSTEM_NUM_STORES
#define aETHERSTEM_NUM_INTERNAL_SLOTS   a40PINSTEM_NUM_INTERNAL_SLOTS
#define aETHERSTEM_NUM_RAM_SLOTS             a40PINSTEM_NUM_RAM_SLOTS
#define aETHERSTEM_NUM_SD_SLOTS               a40PINSTEM_NUM_SD_SLOTS

#define aETHERSTEM_NUM_A2D                         a40PINSTEM_NUM_A2D
#define aETHERSTEM_NUM_APPS                       a40PINSTEM_NUM_APPS
#define aETHERSTEM_NUM_CLOCK                     a40PINSTEM_NUM_CLOCK
#define aETHERSTEM_NUM_DIG                         a40PINSTEM_NUM_DIG
#define aETHERSTEM_NUM_I2C                         a40PINSTEM_NUM_I2C
#define aETHERSTEM_NUM_POINTERS               a40PINSTEM_NUM_POINTERS
#define aETHERSTEM_NUM_SERVOS                   a40PINSTEM_NUM_SERVOS
#define aETHERSTEM_NUM_TIMERS                   a40PINSTEM_NUM_TIMERS



class aEtherStem : public a40PinModule
{
public:
    
    aEtherStem(const uint8_t module = aETHERSTEM_MODULE_ADDRESS,
               bool bAutoNetworking = true) :
    a40PinModule(module, bAutoNetworking)
    {
        
    }
    
};

#endif
