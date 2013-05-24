//
//  RCFractionView.h
//  RCCore
//
//  Created by Ben Hirashima on 5/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface RCFractionView : UIView
- (void)setNominator:(int)nom andDenominator:(int)denom;
- (void)parseFraction:(NSString*)fractionString;
@end
