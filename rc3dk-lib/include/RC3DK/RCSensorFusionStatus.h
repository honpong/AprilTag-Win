//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCSensorFusionInternals.h"

/** Represents the status of the sensor fusion engine. */
@interface RCSensorFusionStatus : NSObject

/** Specifies the internal state of RCSensorFusion.
 
 Sensor fusion states will change asynchronously with the requests made to RCSensorFusion. This property always describes the state the filter was in for the given update. Possible states are:
 
 - RCSensorFusionStateInactive - RCSensorFusion is inactive.
 - RCSensorFusionStateStaticCalibration - RCSensorFusion is in static calibration mode. The device should not be moved or touched. Progress is available on [RCSensorFusionStatus calibrationProgress].
 - RCSensorFusionStateSteadyInitialization - startSensorFusionWithDevice: has been called, and RCSensorFusion is in the handheld steady initialization phase. Progress is available on [RCSensorFusionStatus calibrationProgress].
 - RCSensorFusionStateDynamicInitialization - startSensorFusionUnstableWithDevice: has been called, and RCSensorFusion is in the handheld dynamic initialization phase. Progress is available on [RCSensorFusionStatus calibrationProgress].
 - RCSensorFusionStateRunning - RCSensorFusion is active and updates are being provided with all data.
 */
@property (nonatomic, readonly) RCSensorFusionRunState runState;

/** Indicates the progress in the current runState.
 
 If appropriate for the runState, such as RCSensorFusionRunStateStaticCalibration, the value of this property will be between 0 and 1 to indicate the progress of the action, such as calibration or initialization.
 */
@property (nonatomic, readonly) float progress;

/** Indicates the error, if any, that occurred.
 
 If a state change has occurred without an associated error, this property will be set to RCSensorFusionErrorCodeNone.
 
 - RCSensorFusionErrorCodeNone = 0 - No error has occurred.
 - RCSensorFusionErrorCodeVision = 1 - No visual features were detected in the most recent image. This is normal in some circumstances, such as quick motion or if the device temporarily looks at a blank wall. However, if this is received repeatedly, it may indicate that the camera is covered or it is too dark. RCSensorFusion will continue.
 - RCSensorFusionErrorCodeTooFast = 2 - The device moved more rapidly than expected for typical handheld motion. This may indicate that RCSensorFusion has failed and is providing invalid data. RCSensorFusion will continue.
 - RCSensorFusionErrorCodeOther = 3 - A fatal internal error has occured. Please contact RealityCap and provide [RCSensorFusionStatus statusCode] from the status property of the last received RCSensorFusionData object. RCSensorFusion will be reset.
 - RCSensorFusionErrorCodeLicense = 4 - A license error indicates that the license has not been properly validated, or needs to be validated again.
 */
@property (nonatomic, readonly) RCSensorFusionErrorCode errorCode;

/** You will not typically need to instantiate this class yourself. */
- (id) initWithRunState:(RCSensorFusionRunState)runState withProgress:(float)progress withErrorCode:(RCSensorFusionErrorCode)error;

@end