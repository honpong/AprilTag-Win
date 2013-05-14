//
//  TMTickMarksLayerDelegate.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/14/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMTickMarksLayerDelegate.h"

#define LAYER_HEIGHT 65 //need this because we can't get an accurate layer height for some reason
#define INCHES_PER_CM 0.393700787

@implementation TMTickMarksLayerDelegate
{
    float screenWidthCM;
    Units units;
}

- (id) initWithPhysWidth:(float)widthCM withUnits:(Units)gUnits
{
    self = [super init];
    
    if (self)
    {
        screenWidthCM = widthCM;
        units = gUnits;
    }
    
    return self;
}

- (void) drawLayer:(CALayer *)layer inContext:(CGContextRef)context
{
    if (units == UnitsImperial)
    {
        float screenWidthIn = screenWidthCM * INCHES_PER_CM + 1;
        int wholeInches = ceil(screenWidthIn);
        float pixelsPerInch = layer.frame.size.width / screenWidthIn;
              
        [self drawTickSet:context withOffset:0 withTickCount:wholeInches withSpacing:pixelsPerInch withTickHeight:16];
        [self drawTickSet:context withOffset:pixelsPerInch / 2 withTickCount:wholeInches withSpacing:pixelsPerInch withTickHeight:12];
        [self drawTickSet:context withOffset:pixelsPerInch / 4 withTickCount:wholeInches * 2 withSpacing:pixelsPerInch / 2 withTickHeight:8];
        [self drawTickSet:context withOffset:pixelsPerInch / 8 withTickCount:wholeInches * 4 withSpacing:pixelsPerInch / 4 withTickHeight:4];
    }
    else
    {
        int wholeCM = ceil(screenWidthCM) + 1;
        float pixelsPerCM = layer.frame.size.width / screenWidthCM;
        
        [self drawTickSet:context withOffset:0 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:16];
        [self drawTickSet:context withOffset:pixelsPerCM / 2 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:12];
        [self drawTickSet:context withOffset:pixelsPerCM / 4 withTickCount:wholeCM * 2 withSpacing:pixelsPerCM / 2 withTickHeight:8];
    }
    
    CGContextSetLineWidth(context, 1);
    CGContextSetStrokeColorWithColor(context, [[UIColor blackColor] CGColor]);
    CGContextStrokePath(context);
}

- (void) drawTickSet:(CGContextRef)context withOffset:(float)offsetY withTickCount:(float)count withSpacing:(float)spacing withTickHeight:(int)height
{
    for (int i = 0; i <= count; i++)
    {
        CGContextMoveToPoint(context, offsetY, 0);
        CGContextAddLineToPoint(context, offsetY, height);
        CGContextMoveToPoint(context, offsetY, LAYER_HEIGHT);
        CGContextAddLineToPoint(context, offsetY, LAYER_HEIGHT - height);
        offsetY = offsetY + spacing;
    }    
}

//turns off animations, reduces lag
- (id<CAAction>) actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end
