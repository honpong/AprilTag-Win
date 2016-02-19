//
//  TMLineLayerDelegate.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 7/11/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPLineLayerDelegate.h"
#import "MPLineLayer.h"

@implementation MPLineLayerDelegate

- (void) drawLayer:(CALayer *)layer inContext:(CGContextRef)context
{
    MPLineLayer* lineLayer = (MPLineLayer*)layer;
    
    CGContextMoveToPoint(context, lineLayer.pointA.x, lineLayer.pointA.y);
    CGContextAddLineToPoint(context, lineLayer.pointB.x, lineLayer.pointB.y);
    
    CGContextSetAlpha(context, 1.0);
    CGContextSetLineWidth(context, 2);
    CGContextSetStrokeColorWithColor(context, [[UIColor redColor] CGColor]);
    CGContextStrokePath(context);
}

+ (MPLineLayerDelegate*) sharedInstance
{
    static MPLineLayerDelegate* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [MPLineLayerDelegate new];
    });
    return instance;
}

@end
