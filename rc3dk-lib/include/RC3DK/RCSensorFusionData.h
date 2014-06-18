//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCFeaturePoint.h"
#import "RCTransformation.h"
#import "RCCameraParameters.h"
#import "RCScalar.h"
#import "RCSensorFusionStatus.h"
#import <CoreMedia/CoreMedia.h>

/**
 This class represents a snapshot of the translation, rotation, point cloud, and other sensor fusion data at one moment in time. When processing video data, a new instance of this class is passed to [RCSensorFusionDelegate sensorFusionDidUpdateData:] after each video frame is processed, typically 30 times per second. When video data is not being processed, this will still be sent, but at approximately 10 Hz.
 */
@interface RCSensorFusionData : NSObject

/** An NSArray of RCFeaturePoint objects representing the visual features that were detected by computer vision in the most recent image. */
@property (nonatomic, readonly) NSArray* featurePoints;

/** An RCTransformation object representing the rotation and translation of the device.
 
 The transformation is defined relative to a right-handed coordinate system in which the Z axis points straight up (as defined by gravity) and initial rotation about the Z axis is arbitrary. The translational origin is set when [RCSensorFusion startSensorFusion] is called. */
@property (nonatomic, readonly) RCTransformation* transformation;

/** An RCTransformation object representing the current camera transformation relative to the global reference frame as defined for the transformation property. */
@property (nonatomic, readonly) RCTransformation* cameraTransformation;

/** An RCCameraParameters object representing the current optical (intrinsic) properties of the camera. */
@property (nonatomic, readonly) RCCameraParameters *cameraParameters;

/** An RCScalar object representing the total length of the path the device has traveled.

 Note: This length is not the straight-line distance, which can be computed by calling [RCSensorFusionData.transformation.translation getDistance]. Instead it is the total distance traveled by the device. For example, if the device is moved in a straight line for 1 meter, then returned directly to its starting point, totalPathLength will be 2 meters.
 */
@property (nonatomic, readonly) RCScalar* totalPathLength;

/** A CMSampleBufferRef containing the video frame that was most recently processed.
 
 This image corresponds to the other data contained in this class, so for example, the x and y coordinates in the featurePoints array will correspond to the locations of the features detected in this image. If you are displaying an augmented reality view or video preview with any overlays, use this image. This will ensure that the data you draw with corresponds to the video frame being shown, which minimizes perceptible lag. If video data is not being processed, this property will be nil.
 */
@property (nonatomic, readonly) CMSampleBufferRef sampleBuffer;

/** A uint64_t containing the time in microseconds when this RCSensorFusionData was calculated. */
@property (nonatomic, readonly) uint64_t timestamp;

/** You will not typically need to instantiate this class yourself. */
- (id) initWithTransformation:(RCTransformation*)transformation withCameraTransformation:(RCTransformation*)cameraTransformation withCameraParameters:(RCCameraParameters *)cameraParameters withTotalPath:(RCScalar *)totalPath withFeatures:(NSArray*)featurePoints withSampleBuffer:(CMSampleBufferRef)sampleBuffer withTimestamp:(uint64_t)timestamp;

@end
