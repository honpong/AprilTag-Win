//
//  RCDistanceLabel.h
//  FractionView
//
//  Created by Ben Hirashima on 5/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RCDistanceImperialFractional.h"
#import "RCFractionLabel.h"

@interface RCDistanceLabel : UIView

@property (nonatomic) BOOL centerAlignmentExcludesFraction;
@property (nonatomic) NSString* text;
@property (nonatomic) UIColor* textColor;
@property (nonatomic) UIColor* shadowColor;
@property (nonatomic) NSTextAlignment textAlignment;
@property (nonatomic) UIFont* font;

+ (RCDistanceLabel*) distLabel:(id<RCDistance>)distObj;
+ (RCDistanceLabel*) distLabel:(id<RCDistance>)distObj withFont:(UIFont*)font;
- (void) setDistanceText:(NSString*)dist;
- (void) setDistance:(id<RCDistance>)distObj;
- (void) setDistanceImperialFractional:(RCDistanceImperialFractional*)distObj;

@end
