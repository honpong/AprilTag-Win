//
//  MPUndoOverlay.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 8/1/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MPTintTouchButton.h"

@protocol MPUndoOverlayDelegate <NSObject>

- (void) handleUndoButton;
- (void) handleUndoPeriodExpired;

@end

@interface MPUndoOverlay : UIView

@property (nonatomic) id<MPUndoOverlayDelegate> delegate;
@property (nonatomic, readonly) UILabel* messageLabel;
@property (nonatomic, readonly) MPTintTouchButton* undoButton;

- (id) initWithMessage:(NSString*)message;
- (void) showWithDuration:(NSTimeInterval)duration;
- (void) hide;

@end
