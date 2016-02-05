//
//  TMLineLayer.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 7/15/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>
#import "TMPoint+TMPointExt.h"

@interface TMLineLayer : CALayer

@property (nonatomic) CGPoint pointA;
@property (nonatomic) CGPoint pointB;

- (id) initWithPointA:(CGPoint)pointA andPointB:(CGPoint)pointB;

@end
