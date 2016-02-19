//
//  RCTipView.h
//  RCCore
//
//  Created by Ben Hirashima on 9/8/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@protocol RCTipViewDelegate <NSObject>

- (void) tipIndexUpdated:(int)index;

@end

@interface RCTipView : UILabel

@property (weak, nonatomic) id<RCTipViewDelegate> delegate;
@property (nonatomic) NSArray* tips;
@property (nonatomic) int currentTipIndex;

- (void) showNextTip;
- (void) showTip:(int)index;

@end
