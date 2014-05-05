//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

/** Represents the status of the sensor fusion engine. */
@interface RCSensorFusionStatus : NSObject

/** If sensor fusion is calibrating (a call to [RCSensorFusion startStaticCalibration] was made), the value of this property will be between 0 and 1. When it reaches 1 or greater, sensor fusion can be considered well calibrated. When starting video processing, this property also reflects the progress of the initialization time during which the user must hold the device steady. */
@property (nonatomic, readonly) float calibrationProgress;

/** An internal code representing the status of the sensor fusion engine.
 
 More useful error information is provide by the RCSensorFusionError object pased to  [RCSensorFusionDelegate sensorFusionError:] */
@property (nonatomic, readonly) int statusCode;

/** Indicates that the device is being held relatively steady, with a low linear and angular velocity. */
@property (nonatomic, readonly, getter=isSteady) BOOL steady;

/** You will not typically need to instantiate this class yourself. */
- (id) initWithProgress:(float)initializationProgress withStatusCode:(int)statusCode withIsSteady:(BOOL)isSteady;

@end