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

@interface RCDistanceLabel : UILabel

@property (readonly) UILabel* distanceLabel;
@property (readonly) RCFractionLabel* fractionLabel;
@property (readonly) UILabel* symbolLabel;
@property (nonatomic) BOOL centerAlignmentExcludesFraction;

+ (RCDistanceLabel*) distLabel:(id<RCDistance>)distObj withFrame:(CGRect)frame;
+ (RCDistanceLabel*) distLabel:(id<RCDistance>)distObj withFrame:(CGRect)frame withFont:(UIFont*)font;
- (void) setDistanceText:(NSString*)dist;
- (void) setDistance:(id<RCDistance>)distObj;
- (void) setDistanceImperialFractional:(RCDistanceImperialFractional*)distObj;

@end
