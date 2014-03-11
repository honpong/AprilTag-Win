//
//  MotionManager.h
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <RC3DK/RC3DK.h>
#import <CoreMotion/CoreMotion.h>

/** Handles getting IMU data from the system, and passes it directly to the RCSensorFusion shared instance.
 
 This class is identical to RCMotionManager, included in the 3DK framework. 
 */
@interface MotionManager : NSObject

@property CMMotionManager* cmMotionManager;

- (BOOL) startMotionCapture;
- (void) stopMotionCapture;
- (BOOL) isCapturing;

+ (MotionManager *) sharedInstance;

@end
