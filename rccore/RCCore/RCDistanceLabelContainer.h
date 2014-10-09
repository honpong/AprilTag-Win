//
//  RCDistanceLabelContainer.h
//  RCCore
//
//  Created by Ben Hirashima on 10/2/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RCFractionLabel.h"

@interface RCDistanceLabelContainer : UIView

@property (readonly) UILabel* distanceLabel;
@property (readonly) RCFractionLabel* fractionLabel;
@property (readonly) UILabel* symbolLabel;
@property (nonatomic) BOOL centerAlignmentExcludesFraction;
@property (nonatomic) NSTextAlignment textAlignment;

@end
