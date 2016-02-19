//
//  RCVideoPreviewCRT.h
//  RCCore
//
//  Created by Eagle Jones on 9/25/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCVideoPreview.h"

@interface RCVideoPreviewCRT : RCVideoPreview
{
    CGRect crtClosedFrame;
}

/**
 Starts CRT power up animation
 */
- (void) animateOpen:(UIDeviceOrientation) orientation;
/**
 Starts CRT power down animation
 */
- (void) animateClosed:(UIDeviceOrientation)orientation withCompletionBlock:(void(^)(BOOL finished))completion;
/**
 Fades to/from white.
 */
- (void) fadeToWhite:(bool)to fromWhite:(bool)from inSeconds:(float)seconds;

@end
