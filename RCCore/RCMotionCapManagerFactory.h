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

#ifdef DEBUG
- (BOOL)startMotionCapWithQueue:(NSOperationQueue*)queue;
#endif

@property CMMotionManager* motionManager;

@end

@interface RCMotionCapManagerFactory : NSObject

+ (void)setupMotionCap;
+ (void)setupMotionCap:(CMMotionManager*)motionManager;
+ (id<RCMotionCapManager>)getInstance;

#ifdef DEBUG
+ (void)setInstance:(id<RCMotionCapManager>)mockObject;
#endif

@end
