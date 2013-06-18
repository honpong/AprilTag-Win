//
//  TMMotionCapManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCorvisManagerFactory.h"
#import <CoreMotion/CoreMotion.h>

@protocol RCMotionCapManager <NSObject>

- (BOOL)startMotionCap;
- (void)stopMotionCap;
- (BOOL)isCapturing;

@end

@interface RCMotionCapManagerFactory : NSObject

+ (id<RCMotionCapManager>)getInstance;
+ (void)setInstance:(id<RCMotionCapManager>)mockObject;

@end
