//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCFeaturePoint.h"
#import "RCTransformation.h"
#import "RCSensorFusionStatus.h"
#import <CoreMedia/CoreMedia.h>

@interface RCSensorFusionData : NSObject

@property (nonatomic, readonly) RCSensorFusionStatus* status;
@property (nonatomic, readonly) NSArray* featurePoints;
@property (nonatomic, readonly) RCTransformation* transformation;
@property (nonatomic, readonly) CMSampleBufferRef sampleBuffer;

- (id) initWithStatus:(RCSensorFusionStatus*)status withTransformation:(RCTransformation*)transformation withFeatures:(NSArray*)featurePoints withSampleBuffer:(CMSampleBufferRef)sampleBuffer;

@end