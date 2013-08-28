//
//  TMMotionCapManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCSensorFusion.h"
#import <CoreMotion/CoreMotion.h>

@interface RCMotionManager : NSObject

@property CMMotionManager* cmMotionManager;

- (BOOL) startMotionCapture;
- (void) stopMotionCapture;
- (BOOL) isCapturing;

#ifdef DEBUG
- (BOOL) startMotionCapWithQueue:(NSOperationQueue*)queue;
#endif

+ (RCMotionManager *) sharedInstance;

@end
