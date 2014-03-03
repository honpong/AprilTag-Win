//
//  MPContainerView.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 2/27/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MPRotatingView.h"

@interface MPContainerView : UIView <MPRotatingView>

@property (nonatomic) UIResponder* delegate;

@end
