//
//  TMMotionCapManagerFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/17/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <RC3DK/RC3DK.h>
#import <CoreMotion/CoreMotion.h>

@interface MotionManager : NSObject

@property CMMotionManager* cmMotionManager;

- (BOOL) startMotionCapture;
- (void) stopMotionCapture;
- (BOOL) isCapturing;

#ifdef DEBUG
- (BOOL) startMotionCapWithQueue:(NSOperationQueue*)queue;
#endif

+ (MotionManager *) sharedInstance;

@end
