//
//  RCFractionView.h
//  RCCore
//
//  Created by Ben Hirashima on 5/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface RCFractionLabel : UILabel
- (void)setNominator:(int)nominator andDenominator:(int)denominator;
- (void)setFromStringsNominator:(NSString*)nominator andDenominator:(NSString*)denominator;
- (void)parseFraction:(NSString*)fractionString;
@end
