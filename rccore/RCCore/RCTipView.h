//
//  RCTipView.h
//  RCCore
//
//  Created by Ben Hirashima on 9/8/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

@import UIKit;

@interface RCTipView : UILabel

@property (nonatomic) NSArray* tips;

- (void) showNextTip;

@end
