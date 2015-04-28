//
//  RCMotionManager.h
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <CoreMotion/CoreMotion.h>
#import "RC3DK.h"

/** Handles getting IMU data from the system, and passes it directly to the RCSensorFusion shared instance.
 
 */
@interface RCMotionManager : NSObject

@property CMMotionManager* cmMotionManager;

- (BOOL) startMotionCapture;
- (void) stopMotionCapture;
- (BOOL) isCapturing;
- (void) setDataDelegate:(id <RCSensorDataDelegate>)delegate;

#ifdef DEBUG
- (BOOL) startMotionCapWithQueue:(NSOperationQueue*)queue;
#endif

+ (RCMotionManager *) sharedInstance;

@end
