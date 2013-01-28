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
- (BOOL)startMotionCapWithMotionManager:(CMMotionManager*)motionMan
                              withQueue:(NSOperationQueue*)queue
                      withCorvisManager:(id<RCCorvisManager>)corvisManager;
- (void)stopMotionCap;

@end

@interface RCMotionCapManagerFactory

+ (id<RCMotionCapManager>)getMotionCapManagerInstance;
+ (void)setMotionCapManagerInstance:(id<RCMotionCapManager>)mockObject;

@end
