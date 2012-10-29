//
//  RCMotionCap.h
//  RCCore
//
//  Created by Ben Hirashima on 10/29/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <CoreMotion/CoreMotion.h>

@interface RCMotionCap : NSObject
{
	CMMotionManager *_motionMan;
}

- (id)initWithMotionManager: (CMMotionManager*)motionMan;
- (void)startMotionCapture;
- (void)stopMotionCapture;

@end