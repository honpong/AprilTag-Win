//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCCameraData.h"
#import "RCFeaturePoint.h"
#import "RCPosition.h"
#import "RCOrientation.h"
#import "RCSensorFusionStatus.h"

@interface RCSensorFusionData : NSObject

@property (nonatomic, readonly) RCSensorFusionStatus* status;
@property (nonatomic, readonly) RCCameraData* cameraData;
@property (nonatomic, readonly) NSArray* featurePoints;
@property (nonatomic, readonly) RCPosition* position;
@property (nonatomic, readonly) RCOrientation* orientation;

- (id) initWithStatus:(RCSensorFusionStatus*)status withPosition:(RCPosition*)position withOrientation:(RCOrientation*)orientation withFeatures:(RCFeaturePoint*)featurePoints withCameraData:(RCCameraData*)cameraData;

@end