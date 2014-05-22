//
//  RCStereo.h
//  RCCore
//
//  Created by Eagle Jones on 5/20/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "RCFeaturePoint.h"
#import "RCSensorFusion.h"

@interface RCStereo : NSObject

+ (RCStereo *) sharedInstance;

- (void) processFrame:(RCSensorFusionData *) data withFinal:(bool)final;
- (bool) preprocess;
- (RCFeaturePoint *) triangulatePoint:(CGPoint)point;
- (void) reset;

@end

