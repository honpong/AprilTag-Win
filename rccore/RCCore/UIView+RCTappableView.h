//
//  UIView+RCTappableView.h
//  RCCore
//
//  Created by Ben Hirashima on 10/10/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface UIView (RCTappableView)

@property (nonatomic, copy) void (^onSingleTap)();

@end
