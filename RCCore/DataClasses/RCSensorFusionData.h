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
 This class represents a snapshot of the translation, rotation, point cloud, and other sensor fusion data at one moment in time. A new instance of this class is
 passed to [RCSensorFusionDelegate sensorFusionDidUpdate:] thirty times per second.
 */
@interface RCSensorFusionData : NSObject

/** A RCSensorFusionStatus object containing the current status of the sensor fusion engine. */
@property (nonatomic, readonly) RCSensorFusionStatus* status;
/** An array of RCFeaturePoint objects representing the visual features in the scene that are being tracked with computer vision. */
@property (nonatomic, readonly) NSArray* featurePoints;
/** An RCTransformation object representing the distance the device has moved since sensor fusion started, or since the last time [RCSensorFusion markStart] 
 was called. */
@property (nonatomic, readonly) RCTransformation* transformation;
/** An RCTransformation object representing the current camera transformation. */
@property (nonatomic, readonly) RCTransformation* cameraTransformation;
/** An RCCameraParameters object. */
@property (nonatomic, readonly) RCCameraParameters *cameraParameters;
/** An RCScalar object representing the total length of the path the device traveled since sensor fusion started, or since the last time [RCSensorFusion markStart]
 was called. */
@property (nonatomic, readonly) RCScalar* totalPathLength;
/** The video frame that corresponds to the other data contained in this class. Use this for any video preview or augmented reality views you may have. If you 
 are showing any overlays on the video, this ensures that the data you use to position the overlays corresponds to the video frame it's being shown on top of,
 which minimizes perceptable lag. */
@property (nonatomic, readonly) CMSampleBufferRef sampleBuffer;

/** You should not have any reason to instantiate this class yourself. */
- (id) initWithStatus:(RCSensorFusionStatus*)status withTransformation:(RCTransformation*)transformation withCameraTransformation:(RCTransformation*)cameraTransformation withCameraParameters:(RCCameraParameters *)cameraParameters withTotalPath:(RCScalar *)totalPath withFeatures:(NSArray*)featurePoints withSampleBuffer:(CMSampleBufferRef)sampleBuffer;

@end