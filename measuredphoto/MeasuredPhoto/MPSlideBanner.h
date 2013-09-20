//
//  MPSlideBanner.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MPSlideBanner : UIView

- (void) showAnimated;
- (void) hideWithDelay:(float)secs onCompletion:(void (^)(BOOL finished))completionBlock;
- (void) hideInstantly;

@end
