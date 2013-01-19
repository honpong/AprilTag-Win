//
//  TMMotionCapManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMCorvisManagerFactory.h"
#import <CoreMotion/CoreMotion.h>

@protocol TMMotionCapManager <NSObject>

- (void)startMotionCapture;
- (void)stopMotionCapture;

@end

@interface TMMotionCapManagerFactory
+ (id<TMMotionCapManager>)getMotionCapManagerInstance;
+ (void)setMotionCapManagerInstance:(id<TMMotionCapManager>)mockObject;
@end
