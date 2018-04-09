/////////////////////////////////////////////////////////////////////
//                                                                 //
// file: aUSBCSwitch.h	 	  	                           //
//                                                                 //
/////////////////////////////////////////////////////////////////////
//                                                                 //
// description: USBCSwitch C++ Module object.                      //
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

#ifndef __aUSBCSwitch_H__
#define __aUSBCSwitch_H__

#include "BrainStem-all.h"
#include "aProtocoldefs.h"

#define aUSBCSWITCH_MODULE                                      6

#define aUSBCSWITCH_NUM_APPS                                    4
#define aUSBCSWITCH_NUM_POINTERS                                4
#define aUSBCSWITCH_NUM_STORES                                  2
#define   aUSBCSWITCH_NUM_INTERNAL_SLOTS                        12
#define   aUSBCSWITCH_NUM_RAM_SLOTS                             1
#define aUSBCSWITCH_NUM_TIMERS                                  8
#define aUSBCSWITCH_NUM_USB                                     1
#define aUSBCSWITCH_NUM_MUX                                     1
#define   aUSBCSWITCH_NUM_MUX_CHANNELS                          4

//////////////////////////////////////////////////////////////////////////////

// Bit defines for port state UInt32
// use _BIT(usbPortStateXX) from aDefs.h to get bit value. i.e if (state & _BIT(usbPortState))
#define usbPortStateVBUS                               0
#define usbPortStateHiSpeed                            1
#define usbPortStateSBU                                2
#define usbPortStateSS1                                3
#define usbPortStateSS2                                4
#define usbPortStateCC1                                5
#define usbPortStateCC2                                6
#define set_usbPortStateCOM_ORIENT_STATUS(var, state)  ((var & ~(3 << 7 )) | (state << 7))
#define get_usbPortStateCOM_ORIENT_STATUS(var)         ((var &  (3 << 7 )) >> 7)
#define set_usbPortStateMUX_ORIENT_STATUS(var, state)  ((var & ~(3 << 9 )) | (state << 9))
#define get_usbPortStateMUX_ORIENT_STATUS(var)         ((var &  (3 << 9 )) >> 9)
#define set_usbPortStateSPEED_STATUS(var, state)       ((var & ~(3 << 11)) | (state << 11))
#define get_usbPortStateSPEED_STATUS(var)              ((var &  (3 << 11)) >> 11)
#define usbPortStateCCFlip                             13
#define usbPortStateSSFlip                             14
#define usbPortStateSBUFlip                            15
#define set_usbPortStateDaughterCard(var, state)       ((var & ~(7 << 16)) | (state << 16))
#define get_usbPortStateDaughterCard(var)              ((var & (7 << 16)) >> 16)
#define usbPortStateErrorFlag                          19
#define usbPortStateUSB2Boost                          20
#define usbPortStateUSB3Boost                          21
#define usbPortStateConnectionEstablished              22
#define usbPortStateCC1Inject                          26
#define usbPortStateCC2Inject                          27
#define usbPortStateCC1Detect                          28
#define usbPortStateCC2Detect                          29
#define usbPortStateCC1LogicState                      30
#define usbPortStateCC2LogicState                      31

// State defines for 2 bit orientation state elements.
#define usbPortStateOff                                0
#define usbPortStateSideA                              1
#define usbPortStateSideB                              2
#define usbPortStateSideUndefined                      3


using Acroname::BrainStem::Module;
using Acroname::BrainStem::Link;
using Acroname::BrainStem::AppClass;
using Acroname::BrainStem::MuxClass;
using Acroname::BrainStem::PointerClass;
using Acroname::BrainStem::StoreClass;
using Acroname::BrainStem::SystemClass;
using Acroname::BrainStem::TimerClass;
using Acroname::BrainStem::USBClass;


class aUSBCSwitch : public Module
{
public:
    
    aUSBCSwitch(const uint8_t module = aUSBCSWITCH_MODULE,
                bool bAutoNetworking = true) :
    Module(module, bAutoNetworking)
    {
        
        app[0].init(this, 0);
        app[1].init(this, 1);
        app[2].init(this, 2);
        app[3].init(this, 3);
        
        pointer[0].init(this, 0);
        pointer[1].init(this, 1);
        pointer[2].init(this, 2);
        pointer[3].init(this, 3);
        
        store[storeInternalStore].init(this, storeInternalStore);
        store[storeRAMStore].init(this, storeRAMStore);
        
        system.init(this, 0);
        
        mux.init(this, 0);
        
        timer[0].init(this, 0);
        timer[1].init(this, 1);
        timer[2].init(this, 2);
        timer[3].init(this, 3);
        timer[4].init(this, 4);
        timer[5].init(this, 5);
        timer[6].init(this, 6);
        timer[7].init(this, 7);
        
        usb.init(this, 0);
    }
    
    AppClass app[aUSBCSWITCH_NUM_APPS];
    MuxClass mux;
    PointerClass pointer[aUSBCSWITCH_NUM_POINTERS];
    StoreClass store[aUSBCSWITCH_NUM_STORES];
    SystemClass system;
    TimerClass timer[aUSBCSWITCH_NUM_TIMERS];
    USBClass usb;
};


#endif /* aUSBCSwitch_h */
