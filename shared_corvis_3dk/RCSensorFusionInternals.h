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
    /** RCSensorFusion is in static calibration mode. The device should not be moved or touched. Progress is available on [RCSensorFusionStatus calibrationProgress]. */
    RCSensorFusionRunStateStaticCalibration = 1,
    /** startSensorFusionWithDevice: has been called, and RCSensorFusion is in the handheld steady initialization phase. Progress is available on [RCSensorFusionStatus calibrationProgress]. */
    RCSensorFusionRunStateSteadyInitialization = 2,
    /** startSensorFusionUnstableWithDevice: has been called, and RCSensorFusion is in the handheld dynamic initialization phase. Progress is available on [RCSensorFusionStatus calibrationProgress]. */
    RCSensorFusionRunStateDynamicInitialization = 3,
    /** RCSensorFusion is active and updates are being provided with all data. */
    RCSensorFusionRunStateRunning = 4,
} RCSensorFusionRunState;

typedef enum
{
    /** No error has occurred. */
    RCSensorFusionErrorCodeNone = 0,
    /** No visual features were detected in the most recent image. This is normal in some circumstances, such as quick motion or if the device temporarily looks at a blank wall. However, if this is received repeatedly, it may indicate that the camera is covered or it is too dark. RCSensorFusion will continue.*/
    RCSensorFusionErrorCodeVision = 1,
    /** The device moved more rapidly than expected for typical handheld motion. This may indicate that RCSensorFusion has failed and is providing invalid data. RCSensorFusion will continue.*/
    RCSensorFusionErrorCodeTooFast = 2,
    /** A fatal internal error has occured. Please contact RealityCap and provide [RCSensorFusionStatus statusCode] from the status property of the last received RCSensorFusionData object. RCSensorFusion will be reset.*/
    RCSensorFusionErrorCodeOther = 3,
    /** A license error indicates that the license has not been properly validated, or needs to be validated again.*/
    RCSensorFusionErrorCodeLicense = 4
} RCSensorFusionErrorCode;

#endif
