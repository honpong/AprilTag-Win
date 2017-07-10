//
//  RCSensorFusionInternals.h
//  RC3DK
//
//  Created by Eagle Jones on 6/5/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#ifndef RC3DK_RCSensorFusionInternals_h
#define RC3DK_RCSensorFusionInternals_h

// appledoc does not currently support C enums, so the doc comments here are currently unused. these enums are instead documented in RCSensorFusionStatus.h

typedef enum
{
    /** RCSensorFusion is inactive. */
    RCSensorFusionRunStateInactive = 0,
    /** startSensorFusionUnstableWithDevice: has been called, and RCSensorFusion is in the handheld dynamic initialization phase. */
    RCSensorFusionRunStateDynamicInitialization = 3,
    /** RCSensorFusion is active and updates are being provided with all data. */
    RCSensorFusionRunStateRunning = 4,
    /** RCSensorFusion is in inertial only mode. Orientation will be tracked, and internal states will be updated so that initialization will occur more quickly. */
    RCSensorFusionRunStateInertialOnly = 7
} RCSensorFusionRunState;

#endif
