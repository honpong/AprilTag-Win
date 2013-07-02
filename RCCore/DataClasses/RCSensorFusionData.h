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

@property (nonatomic, readonly) RCSensorFusionStatus* status;
@property (nonatomic, readonly) NSArray* featurePoints;
@property (nonatomic, readonly) RCTransformation* transformation;
@property (nonatomic, readonly) RCTransformation* cameraTransformation;
@property (nonatomic, readonly) RCCameraParameters *cameraParameters;
@property (nonatomic, readonly) RCScalar* totalPathLength;
@property (nonatomic, readonly) CMSampleBufferRef sampleBuffer;

- (id) initWithStatus:(RCSensorFusionStatus*)status withTransformation:(RCTransformation*)transformation withCameraTransformation:(RCTransformation*)cameraTransformation withCameraParameters:(RCCameraParameters *)cameraParameters withTotalPath:(RCScalar *)totalPath withFeatures:(NSArray*)featurePoints withSampleBuffer:(CMSampleBufferRef)sampleBuffer;

@end