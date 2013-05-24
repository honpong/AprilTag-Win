//
//  RCDistanceLabel.h
//  FractionView
//
//  Created by Ben Hirashima on 5/22/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RCDistanceImperialFractional.h"
#import "RCFractionView.h"

@interface RCDistanceLabel : UIView

@property UILabel* distanceLabel;
@property RCFractionView* fractionLabel;
@property UILabel* symbolLabel;
@property NSTextAlignment textAlignment;

- (void) setDistanceText:(NSString*)dist;
- (void) setDistanceImperialFractional:(RCDistanceImperialFractional*)distObj;
@end
