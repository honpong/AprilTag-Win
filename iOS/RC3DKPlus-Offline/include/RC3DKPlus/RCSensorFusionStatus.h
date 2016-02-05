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
 
 - RCSensorFusionRunStateInactive - RCSensorFusion is inactive.
 - RCSensorFusionRunStateStaticCalibration - RCSensorFusion is in static calibration mode. The device should not be moved or touched. Progress is available on [RCSensorFusionStatus calibrationProgress].
 - RCSensorFusionRunStateSteadyInitialization - startSensorFusionWithDevice: has been called, and RCSensorFusion is in the handheld steady initialization phase. Progress is available on [RCSensorFusionStatus calibrationProgress].
 - RCSensorFusionRunStateDynamicInitialization - startSensorFusionUnstableWithDevice: has been called, and RCSensorFusion is in the handheld dynamic initialization phase. Progress is available on [RCSensorFusionStatus calibrationProgress].
 - RCSensorFusionRunStateRunning - RCSensorFusion is active and updates are being provided with all data.
 - RCSensorFusionRunStatePortraitCalibration - RCSensorFusion is in handheld portrait calibration mode. The device should be held steady in portrait orientation, perpendicular to the floor. Progress is available on [RCSensorFusionStatus calibrationProgress].
 - RCSensorFusionRunStateLandscapeCalibration - RCSensorFusion is in handheld landscape calibration mode. The device should be held steady in landscape orientation, perpendicular to the floor. Progress is available on [RCSensorFusionStatus calibrationProgress].
 */
@property (nonatomic, readonly) RCSensorFusionRunState runState;

/** Indicates the progress in the current runState.
 
 If appropriate for the runState, such as RCSensorFusionRunStateStaticCalibration, the value of this property will be between 0 and 1 to indicate the progress of the action, such as calibration or initialization.
 */
@property (nonatomic, readonly) float progress;

/** Specifies the overall level of confidence that RCSensorFusion has in its output.
 
 RCSensorFusion requires both visual features and motion to provide accurate measurements. If no visual features are visible, the user doesn't move, or the user moves too slowly, then there will not be enough information available to reliably estimate data. If accuracy is important, apps should prompt the user to move the device with enough velocity to trigger RCSensorFusionConfidenceHigh before beginning a measurement. Possible states are:
 
 - RCSensorFusionConfidenceNone - RCSensorFusion is not currently running (possibly due to failure).
 - RCSensorFusionConfidenceLow - This occurs if no visual features could be detected or tracked.
 - RCSensorFusionConfidenceMedium - This occurs when RCSensorFusion has recently been initialized, or has recovered from having few usable visual features, and continues until the user has moved sufficiently to produce reliable measurements. If the user moves too slowly or features are unreliable, this will not switch to RCSensorFusionConfidenceHigh, and measurements may be unreliable.
 - RCSensorFusionConfidenceHigh - Sufficient visual features and motion have been detected that measurements are likely to be accurate.
 */
@property (nonatomic, readonly) RCSensorFusionConfidence confidence;

/** 
 Indicates the error, if any, that occurred. If a state change has occurred without an associated error, this property will be set to nil. 
 Check this property on every invocation of [RCSensorFusionDelegate sensorFusionDidChangeStatus:]. 
 Check the class of the error to determine which type of error occured. The error class may be RCSensorFusionError or RCLicenseError.
 */
@property (nonatomic, readonly) NSError* error;

/** You will not typically need to instantiate this class yourself. */
- (id) initWithRunState:(RCSensorFusionRunState)runState withProgress:(float)progress withConfidence:(RCSensorFusionConfidence)confidence withError:(NSError*)error;

- (NSDictionary*) dictionaryRepresentation;

@end