//
//  RCCameraParameters.m
//  RCCore
//
//  Created by Eagle Jones on 6/28/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCameraParameters.h"

@implementation RCCameraParameters

- (id) initWithFocalLength:(float)focalLength withOpticalCenterX:(float)opticalCenterX withOpticalCenterY:(float)opticalCenterY withRadialSecondDegree:(float)radialSecondDegree withRadialFourthDegree:(float)radialFourthDegree
{
    if(self = [super init]) {
        _focalLength = focalLength;
        _opticalCenterX = opticalCenterX;
        _opticalCenterY = opticalCenterY;
        _radialSecondDegree = radialSecondDegree;
        _radialFourthDegree = radialFourthDegree;
    }
    return self;
}

- (NSDictionary *)dictionaryRepresentation
{
    NSDictionary* dict = @{
                           @"focalLength": @(self.focalLength),
                           @"opticalCenterX": @(self.opticalCenterX),
                           @"opticalCenterY": @(self.opticalCenterY),
                           @"radialSecondDegree": @(self.radialSecondDegree),
                           @"radialFouthDegree": @(self.radialFourthDegree)
                           };
    return dict;
}

@end
