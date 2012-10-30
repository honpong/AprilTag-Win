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
    struct mapbuffer *_output;
    NSOperationQueue *_queueMotion;
}

- (id)initWithMotionManager: (CMMotionManager*)motionMan withOutput:(struct mapbuffer *) output;
- (void)startMotionCapture;
- (void)stopMotionCapture;

@end