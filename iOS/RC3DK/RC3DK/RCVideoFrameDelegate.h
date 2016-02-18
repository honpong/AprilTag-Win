//
//  RCVideoFrameDelegate.h
//  RCCore
//
//  Created by Ben Hirashima on 4/27/2015.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#import <CoreMedia/CMSampleBuffer.h>

@protocol RCVideoFrameDelegate <NSObject>
@required
    - (void)receiveVideoFrame:(CMSampleBufferRef)sampleBuffer;
@end