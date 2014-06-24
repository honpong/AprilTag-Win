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

/** 
 Indicates the error, if any, that occurred. If a state change has occurred without an associated error, this property will be set to nil. 
 Check this property on every invocation of [RCSensorFusionDelegate sensorFusionDidChangeStatus:]. 
 Check the class of the error to determine which type of error occured. The error class may be RCSensorFusionError or RCLicenseError.
 */
@property (nonatomic, readonly) NSError* error;

/** You will not typically need to instantiate this class yourself. */
- (id) initWithRunState:(RCSensorFusionRunState)runState withProgress:(float)progress withError:(NSError*)error;

@end