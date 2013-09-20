//
//  MPSlideBanner.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPSlideBanner.h"

@implementation MPSlideBanner

- (void) showAnimated
{
    [UIView animateWithDuration: .5
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         self.center = CGPointMake(self.center.x, self.center.y - self.bounds.size.height);
                     }
                     completion:nil];
}

- (void) hideWithDelay:(float)secs onCompletion:(void (^)(BOOL finished))completionBlock
{
    [UIView animateWithDuration: .5
                          delay: secs
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         self.center = CGPointMake(self.center.x, self.center.y + self.bounds.size.height);
                     }
                     completion:completionBlock];
}

- (void) hideInstantly
{
    self.center = CGPointMake(self.center.x, self.center.y + self.bounds.size.height);
}

@end
