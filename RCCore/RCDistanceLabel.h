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

@property UILabel* distanceLabel;
@property RCFractionLabel* fractionLabel;
@property UILabel* symbolLabel;

- (void) setDistanceText:(NSString*)dist;
- (void) setDistance:(id<RCDistance>)distObj;
- (void) setDistanceImperialFractional:(RCDistanceImperialFractional*)distObj;
@end
