//
//  RCVideoCap.h
//  RCCore
//
//  Created by Ben Hirashima on 10/29/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>
#import "cor.h"

@interface RCVideoCap : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>
{
	AVCaptureSession *session;
    struct mapbuffer *_output;
}

- (AVCaptureSession*)startVideoCap;
- (void)stopVideoCap;
@end
