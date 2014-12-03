//  CATInstructionsView.h
//  TrackLinks
//
//  Created by Ben Hirashima on 2/4/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

@protocol MPInstructionsViewDelegate <NSObject>

- (void) moveComplete;

@end

@class RCTransformation;

@interface CATInstructionsView : UIView

@property (nonatomic) id<MPInstructionsViewDelegate> delegate;

- (void) updateDotPosition:(RCTransformation*)transformation withDepth:(float)depth;
- (void) moveDotToCenter;

@end
