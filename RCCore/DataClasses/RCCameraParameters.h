//
//  RCCameraParameters.h
//  RCCore
//
//  Created by Eagle Jones on 6/28/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface RCCameraParameters : NSObject

@property(readonly,nonatomic) float focalLength;
@property(readonly,nonatomic) float opticalCenterX;
@property(readonly,nonatomic) float opticalCenterY;
@property(readonly,nonatomic) float radialSecondDegree;
@property(readonly,nonatomic) float radialFourthDegree;

- (id) initWithFocalLength:(float)focalLength withOpticalCenterX:(float)opticalCenterX withOpticalCenterY:(float)opticalCenterY withRadialSecondDegree:(float)radialSecondDegree withRadialFourthDegree:(float)radialFouthDegree;

@end
