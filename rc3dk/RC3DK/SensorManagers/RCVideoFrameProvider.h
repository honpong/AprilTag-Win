//
//  RCVideoFrameProvider.h
//  RCCore
//
//  Created by Eagle Jones on 9/22/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCVideoFrameDelegate.h"

@protocol RCVideoFrameProvider <NSObject>
@required
    @property (readonly) NSArray* delegates;

    - (void) addDelegate:(id <RCVideoFrameDelegate>)delegate;
    - (void) removeDelegate:(id <RCVideoFrameDelegate>)delegate;
@end
