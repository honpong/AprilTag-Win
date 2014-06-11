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
 
 Sensor fusion states will change asynchronously with the requests made to RCSensorFusion. This property always describes the state the filter was in for the given update.
 */
@property (nonatomic, readonly) RCSensorFusionRunState runState;

/** Indicates the progress in the current runState.
 
 If appropriate for the runState, such as RCSensorFusionRunStateStaticCalibration, the value of this property will be between 0 and 1 to indicate the progress of the action, such as calibration or initialization.
 */
@property (nonatomic, readonly) float progress;

/** Indicates the error, if any, that occurred.
 
 If a state change has occurred without an associated error, this property will be set to RCSensorFusionErrorCodeNone.
 */
@property (nonatomic, readonly) RCSensorFusionErrorCode errorCode;

/** You will not typically need to instantiate this class yourself. */
- (id) initWithRunState:(RCSensorFusionRunState)runState withProgress:(float)progress withErrorCode:(RCSensorFusionErrorCode)error;

@end