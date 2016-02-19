//
//  RCFractionView.h
//  RCCore
//
//  Created by Ben Hirashima on 5/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface RCFractionLabel : UIView

@property (nonatomic) UIFont* font;
@property (nonatomic) UIColor* textColor;
@property (nonatomic) UIColor* shadowColor;
@property (nonatomic) CGSize shadowOffset;

- (void)setNominator:(int)nominator andDenominator:(int)denominator;
- (void)setFromStringsNominator:(NSString*)nominator andDenominator:(NSString*)denominator;
- (void)parseFraction:(NSString*)fractionString;

@end
