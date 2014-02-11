//
//  MPInstructionsView.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/4/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPView.h"
#import "MPRotatingView.h"
#import "MPPaddedLabel.h"

@interface MPInstructionsView : MPView <MPRotatingView>

@property (nonatomic) MPPaddedLabel* messageBox;

- (void) moveDotToX:(CGFloat)x andY:(CGFloat)y;

@end
