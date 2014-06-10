//
//  RCSensorFusionState.h
//  RC3DK
//
//  Created by Eagle Jones on 6/5/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#ifndef RC3DK_RCSensorFusionState_h
#define RC3DK_RCSensorFusionState_h

//TODO - make sure RCSensorFusionState.h is processed for documentation

/**
 Represents the internal state of RCSensorFusion.
 */
typedef enum {
    /** RCSensorFusion is inactive. */
    RCSensorFusionStateInactive = 0,
    /** RCSensorFusion is in static calibration mode. The device should not be moved or touched. Progress is available on [RCSensorFusionStatus calibrationProgress]. */
    RCSensorFusionStateStaticCalibration = 1,
    /** startSensorFusionWithDevice: has been called, and RCSensorFusion is in the handheld steady initialization phase. Progress is available on [RCSensorFusionStatus calibrationProgress]. */
    RCSensorFusionStateSteadyInitialization = 2,
    /** startSensorFusionUnstableWithDevice: has been called, and RCSensorFusion is in the handheld dynamic initialization phase. Progress is available on [RCSensorFusionStatus calibrationProgress]. */
    RCSensorFusionStateDynamicInitialization = 3,
    /** RCSensorFusion is active and updates are being provided with all data. */
    RCSensorFusionStateRunning = 4,
} RCSensorFusionState;

#endif
