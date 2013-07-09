//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

/** Represents the status of the sensor fusion engine. */
@interface RCSensorFusionStatus : NSObject

/** If sensor fusion is initializing or calibrating, the value of this property will be between 0 and 1. When it reaches 1, sensor fusion is initialized. */
@property (nonatomic, readonly) float initializationProgress;

/** An internal code representing the status of the sensor fusion engine.
 
 More useful error information is provide by the RCSensorFusionError object pased to  [RCSensorFusionDelegate sensorFusionError:] */
@property (nonatomic, readonly) int statusCode;

/** Indicates that the device is being held relatively steady, with a low linear and angular velocity. */
@property (nonatomic, readonly, getter=isSteady) BOOL steady;

/** You will not typically need to instantiate this class yourself. */
- (id) initWithProgress:(float)initializationProgress withStatusCode:(int)statusCode withIsSteady:(BOOL)isSteady;

@end