//
//  RCSensorFusionState.h
//  RC3DK
//
//  Created by Eagle Jones on 6/5/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#ifndef RC3DK_RCSensorFusionState_h
#define RC3DK_RCSensorFusionState_h

typedef enum {
    RCSensorFusionStateInactive = 0,
    RCSensorFusionStateStaticCalibration = 1,
    RCSensorFusionStateSteadyInitialization = 2,
    RCSensorFusionStateDynamicInitialization = 3,
    RCSensorFusionStateRunning = 4,
} RCSensorFusionState;

typedef enum {
    RCSensorFusionErrorCodeNone = 0,
    RCSensorFusionErrorCodeVision = 1,
    RCSensorFusionErrorCodeTooFast = 2,
    RCSensorFusionErrorCodeOther = 3,
    RCSensorFusionErrorCodeLicense = 4
} RCSensorFusionErrorCode;

#endif
