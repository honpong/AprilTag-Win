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

- (bool)startMotionCap;
- (void)stopMotionCap;

@end

@interface RCMotionCapManagerFactory

+ (void)setupMotionCap;
+ (void)setupMotionCap:(id<RCCorvisManager>)corvisManager;
+ (id<RCMotionCapManager>)getMotionCapManagerInstance;
+ (void)setMotionCapManagerInstance:(id<RCMotionCapManager>)mockObject;

@end
