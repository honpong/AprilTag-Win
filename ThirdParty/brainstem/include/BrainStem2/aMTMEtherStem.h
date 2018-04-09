/////////////////////////////////////////////////////////////////////
//                                                                 //
// file: aMTMEtherStem.h                                            //
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

#ifndef __aMTMEtherStem_H__
#define __aMTMEtherStem_H__

#include "BrainStem-all.h"
#include "aProtocoldefs.h"

#define aMTM_ETHERSTEM_MODULE_BASE_ADDRESS  aMTM_STEM_MODULE_BASE_ADDRESS

#define aMTM_ETHERSTEM_NUM_STORES                    aMTM_STEM_NUM_STORES
#define   aMTM_ETHERSTEM_NUM_INTERNAL_SLOTS  aMTM_STEM_NUM_INTERNAL_SLOTS
#define   aMTM_ETHERSTEM_NUM_RAM_SLOTS            aMTM_STEM_NUM_RAM_SLOTS
#define   aMTM_ETHERSTEM_NUM_SD_SLOTS              aMTM_STEM_NUM_SD_SLOTS

#define aMTM_ETHERSTEM_NUM_A2D                          aMTM_STEM_NUM_A2D
#define aMTM_ETHERSTEM_NUM_APPS                        aMTM_STEM_NUM_APPS
#define aMTM_ETHERSTEM_NUM_CLOCK                      aMTM_STEM_NUM_CLOCK
#define aMTM_ETHERSTEM_NUM_DIG                          aMTM_STEM_NUM_DIG
#define aMTM_ETHERSTEM_NUM_I2C                          aMTM_STEM_NUM_I2C
#define aMTM_ETHERSTEM_NUM_POINTERS                aMTM_STEM_NUM_POINTERS
#define aMTM_ETHERSTEM_NUM_SERVOS                    aMTM_STEM_NUM_SERVOS
#define aMTM_ETHERSTEM_NUM_TIMERS                    aMTM_STEM_NUM_TIMERS


class aMTMEtherStem : public aMTMStemModule
{
public:

    aMTMEtherStem(const uint8_t module = aMTM_ETHERSTEM_MODULE_BASE_ADDRESS,
                  bool bAutoNetworking = true) :
    aMTMStemModule(module, bAutoNetworking)
    {

    }

};

#endif
