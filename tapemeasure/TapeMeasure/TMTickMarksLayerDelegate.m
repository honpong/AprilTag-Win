//
//  TMTickMarksLayerDelegate.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/14/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMTickMarksLayerDelegate.h"

#define LAYER_HEIGHT 65 //need this because we can't get an accurate layer height for some reason
#define SCREEN_PIXEL_WIDTH 320

@implementation TMTickMarksLayerDelegate
{
    float screenWidthMeters;
    int screenWidthCM;
    Units units;
}

- (id) initWithWidthMeters:(float)screenWidthMeters_ withUnits:(Units)units_
{
    self = [super init];
    
    if (self)
    {
        screenWidthMeters = screenWidthMeters_;
        screenWidthCM = screenWidthMeters * 100;
        units = units_;
    }
    
    return self;
}

- (void) drawLayer:(CALayer *)layer inContext:(CGContextRef)context
{
    if (units == UnitsImperial)
    {
        float screenWidthIn = screenWidthMeters * INCHES_PER_METER;
        int wholeInches = ceil(screenWidthIn) + 2;
        float pixelsPerInch = SCREEN_PIXEL_WIDTH / screenWidthIn;
              
        [self drawTickSet:context withOffset:0 withTickCount:wholeInches withSpacing:pixelsPerInch withTickHeight:16];
        [self drawTickSet:context withOffset:pixelsPerInch / 2 withTickCount:wholeInches withSpacing:pixelsPerInch withTickHeight:14];
        [self drawTickSet:context withOffset:pixelsPerInch / 4 withTickCount:wholeInches * 2 withSpacing:pixelsPerInch / 2 withTickHeight:10];
        [self drawTickSet:context withOffset:pixelsPerInch / 8 withTickCount:wholeInches * 4 withSpacing:pixelsPerInch / 4 withTickHeight:6];
        [self drawTickSet:context withOffset:pixelsPerInch / 16 withTickCount:wholeInches * 8 withSpacing:pixelsPerInch / 8 withTickHeight:3];
    }
    else
    {
        int wholeCM = ceil(screenWidthCM) + 1;
        float pixelsPerCM = SCREEN_PIXEL_WIDTH / screenWidthCM;
        float pixelsPerMM = pixelsPerCM / 10;
        
        [self drawTickSet:context withOffset:0 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:14];
        [self drawTickSet:context withOffset:pixelsPerCM / 2 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:8];
        [self drawTickSet:context withOffset:pixelsPerMM     withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:4];
        [self drawTickSet:context withOffset:pixelsPerMM * 2 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:4];
        [self drawTickSet:context withOffset:pixelsPerMM * 3 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:4];
        [self drawTickSet:context withOffset:pixelsPerMM * 4 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:4];
        [self drawTickSet:context withOffset:pixelsPerMM * 6 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:4];
        [self drawTickSet:context withOffset:pixelsPerMM * 7 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:4];
        [self drawTickSet:context withOffset:pixelsPerMM * 8 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:4];
        [self drawTickSet:context withOffset:pixelsPerMM * 9 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:4];
    }
    
    CGContextSetShouldAntialias(context, NO);
    CGContextSetLineWidth(context, 1);
    CGContextSetStrokeColorWithColor(context, [[UIColor yellowColor] CGColor]);
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
