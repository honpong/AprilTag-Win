//
//  MPExpandingCircleAnimationView.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 12/3/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MPExpandingCircleAnimationView : UIView

@property (nonatomic) CGFloat beginningCircleRadius;
@property (nonatomic) CGFloat endingCircleRadius;

- (void) startHighlightAnimation;
- (void) stopHighlightAnimation;

@end
