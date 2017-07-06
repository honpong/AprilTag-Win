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
    /** startSensorFusionWithDevice: has been called, and RCSensorFusion is in the handheld steady initialization phase. Progress is available on [RCSensorFusionStatus calibrationProgress]. */
    RCSensorFusionRunStateSteadyInitialization = 2,
    /** startSensorFusionUnstableWithDevice: has been called, and RCSensorFusion is in the handheld dynamic initialization phase. Progress is available on [RCSensorFusionStatus calibrationProgress]. */
    RCSensorFusionRunStateDynamicInitialization = 3,
    /** RCSensorFusion is active and updates are being provided with all data. */
    RCSensorFusionRunStateRunning = 4,
    /** RCSensorFusion is in inertial only mode. Orientation will be tracked, and internal states will be updated so that initialization will occur more quickly. */
    RCSensorFusionRunStateInertialOnly = 7
} RCSensorFusionRunState;

typedef enum
{
    /** No error has occurred. */
    RCSensorFusionErrorCodeNone = 0,
    /** No visual features were detected in the most recent image. This is normal in some circumstances, such as quick motion or if the device temporarily looks at a blank wall. RCSensorFusion will continue. When features are detected again, [RCSensorFusionDelegate sensorFusionDidChangeStatus] will be called again with RCSensorFusionStatus.error set to nil. If the time between these two events is more than a couple seconds, it is likely to be unrecoverable.*/
    RCSensorFusionErrorCodeVision = 1,
    /** The device moved more rapidly than expected for typical handheld motion. This may indicate that RCSensorFusion has failed and is providing invalid data. RCSensorFusion will be reset.*/
    RCSensorFusionErrorCodeTooFast = 2,
    /** A fatal internal error has occured. Please contact RealityCap and provide [RCSensorFusionStatus statusCode] from the status property of the last received RCSensorFusionData object. RCSensorFusion will be reset.*/
    RCSensorFusionErrorCodeOther = 3,
    /** A license error indicates that the license has not been properly validated, or needs to be validated again.*/
    RCSensorFusionErrorCodeLicense = 4
} RCSensorFusionErrorCode;

typedef enum
{
    /** RCSensorFusion is not currently running (possibly due to failure). */
    RCSensorFusionConfidenceNone = 0,
    /** The data has low confidence. This occurs if no visual features could be detected or tracked. */
    RCSensorFusionConfidenceLow = 1,
    /** The data has medium confidence. This occurs when RCSensorFusion has recently been initialized, or has recovered from having few usable visual features, and continues until the user has moved sufficiently to produce reliable measurements. If the user moves too slowly or features are unreliable, this will not switch to RCSensorFusionConfidenceHigh, and measurements may be unreliable. */
    RCSensorFusionConfidenceMedium = 2,
    /** Sufficient visual features and motion have been detected that measurements are likely to be accurate. */
    RCSensorFusionConfidenceHigh = 3
} RCSensorFusionConfidence;

#endif
