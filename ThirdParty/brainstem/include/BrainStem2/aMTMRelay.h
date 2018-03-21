/////////////////////////////////////////////////////////////////////
//                                                                 //
// file: aMTMRelay.h	 	  	                                       //
//                                                                 //
/////////////////////////////////////////////////////////////////////
//                                                                 //
// description: BrainStem MTM-RELAY module object.                 //
//                                                                 //
// build number: source                                            //
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

#ifndef __aMTMRelay_H__
#define __aMTMRelay_H__

#include "BrainStem-all.h"
#include "aProtocoldefs.h"

#define aMTMRELAY_MODULE_BASE_ADDRESS                              12

#define aMTMRELAY_NUM_APPS                                          4
#define aMTMRELAY_NUM_DIGITALS                                      4
#define aMTMRELAY_NUM_I2C                                           1
#define aMTMRELAY_NUM_POINTERS                                      4
#define aMTMRELAY_NUM_RELAYS                                        4
#define aMTMRELAY_NUM_STORES                                        2
#define   aMTMRELAY_NUM_INTERNAL_SLOTS                             12
#define   aMTMRELAY_NUM_RAM_SLOTS                                   1
#define aMTMRELAY_NUM_TIMERS                                        8


using Acroname::BrainStem::Module;
using Acroname::BrainStem::Link;
using Acroname::BrainStem::AppClass;
using Acroname::BrainStem::DigitalClass;
using Acroname::BrainStem::I2CClass;
using Acroname::BrainStem::PointerClass;
using Acroname::BrainStem::RelayClass;
using Acroname::BrainStem::StoreClass;
using Acroname::BrainStem::SystemClass;
using Acroname::BrainStem::TimerClass;


class aMTMRelay : public Module
{
public:
    aMTMRelay(const uint8_t module = aMTMRELAY_MODULE_BASE_ADDRESS,
              bool bAutoNetworking = true) :
    Module(module, bAutoNetworking)
    {
        
        app[0].init(this, 0);
        app[1].init(this, 1);
        app[2].init(this, 2);
        app[3].init(this, 3);

        digital[0].init(this, 0);
        digital[1].init(this, 1);
        digital[2].init(this, 2);
        digital[3].init(this, 3);

        i2c[0].init(this, 0);
        
        pointer[0].init(this, 0);
        pointer[1].init(this, 1);
        pointer[2].init(this, 2);
        pointer[3].init(this, 3);

        relay[0].init(this, 0);
        relay[1].init(this, 1);
        relay[2].init(this, 2);
        relay[3].init(this, 3);

        store[storeInternalStore].init(this, storeInternalStore);
        store[storeRAMStore].init(this, storeRAMStore);

        system.init(this, 0);

        timer[0].init(this, 0);
        timer[1].init(this, 1);
        timer[2].init(this, 2);
        timer[3].init(this, 3);
        timer[4].init(this, 4);
        timer[5].init(this, 5);
        timer[6].init(this, 6);
        timer[7].init(this, 7);

    }

    AppClass app[aMTMRELAY_NUM_APPS];
    DigitalClass digital[aMTMRELAY_NUM_DIGITALS];
    I2CClass i2c[aMTMRELAY_NUM_I2C];
    PointerClass pointer[aMTMRELAY_NUM_POINTERS];
    RelayClass relay[aMTMRELAY_NUM_RELAYS];
    StoreClass store[aMTMRELAY_NUM_STORES];
    SystemClass system;
    TimerClass timer[aMTMRELAY_NUM_TIMERS];

};

#endif /* __aMTMRelay_H__ */
