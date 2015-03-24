//
//  RCVideoFrameProvider.h
//  RCCore
//
//  Created by Eagle Jones on 9/22/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#ifndef RCCore_RCVideoFrameProvider_h
#define RCCore_RCVideoFrameProvider_h

#import <CoreMedia/CMSampleBuffer.h>

@protocol RCVideoFrameDelegate <NSObject>
@required
- (void) displaySampleBuffer:(CMSampleBufferRef)sampleBuffer;
@end

@protocol RCVideoFrameProvider <NSObject>
@property id<RCVideoFrameDelegate> delegate;
@end

#endif
