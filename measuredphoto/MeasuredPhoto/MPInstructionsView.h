//
//  MPInstructionsView.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/4/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPView.h"

@protocol MPInstructionsViewDelegate <NSObject>

- (void) moveComplete;

@end

@class RCTransformation;

@interface MPInstructionsView : MPView

@property (nonatomic) id<MPInstructionsViewDelegate> delegate;

- (void) updateDotPosition:(RCTransformation*)transformation;
- (void) moveDotToCenter;

@end
