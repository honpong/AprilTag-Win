//
//  TMTickMarksLayerDelegate.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/14/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMTickMarksLayerDelegate.h"

#define LAYER_HEIGHT 80 //need this because we can't get an accurate layer height for some reason
#define SCREEN_PIXEL_WIDTH 320

static const CGFloat tickScaleFactor = 2;

@implementation TMTickMarksLayerDelegate
{
    CGFloat screenWidthMeters;
    CGFloat screenWidthCM;
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
    CGContextSetShouldAntialias(context, NO);
    
    [self drawTapeWithColor:[UIColor colorWithRed:.855 green:.648 blue:.164 alpha:1.] withLineWidth:2 withOffset:0 inContext:context];
    [self drawTapeWithColor:[UIColor colorWithHue:0 saturation:0 brightness:.2 alpha:1] withLineWidth:1 withOffset:1 inContext:context];
}

- (void) drawTapeWithColor:(UIColor*)color withLineWidth:(CGFloat)lineWidth withOffset:(CGFloat)offset inContext:(CGContextRef)context
{
    if (units == UnitsImperial)
    {
        CGFloat screenWidthIn = screenWidthMeters * INCHES_PER_METER;
        int wholeInches = ceil(screenWidthIn) + 2;
        CGFloat pixelsPerInch = SCREEN_PIXEL_WIDTH / screenWidthIn;
        
        [self drawTickSet:context withOffset:offset + 0 withTickCount:wholeInches withSpacing:pixelsPerInch withTickHeight:tickScaleFactor * 16];
        [self drawTickSet:context withOffset:offset + pixelsPerInch / 2 withTickCount:wholeInches withSpacing:pixelsPerInch withTickHeight:tickScaleFactor * 14];
        [self drawTickSet:context withOffset:offset + pixelsPerInch / 4 withTickCount:wholeInches * 2 withSpacing:pixelsPerInch / 2 withTickHeight:tickScaleFactor * 10];
        [self drawTickSet:context withOffset:offset + pixelsPerInch / 8 withTickCount:wholeInches * 4 withSpacing:pixelsPerInch / 4 withTickHeight:tickScaleFactor * 6];
        [self drawTickSet:context withOffset:offset + pixelsPerInch / 16 withTickCount:wholeInches * 8 withSpacing:pixelsPerInch / 8 withTickHeight:tickScaleFactor * 3];
    }
    else
    {
        int wholeCM = ceil(screenWidthCM) + 1;
        CGFloat pixelsPerCM = SCREEN_PIXEL_WIDTH / screenWidthCM;
        CGFloat pixelsPerMM = pixelsPerCM / 10;
        
        [self drawTickSet:context withOffset:offset + 0 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:tickScaleFactor * 14];
        [self drawTickSet:context withOffset:offset + pixelsPerCM / 2 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:tickScaleFactor * 8];
        [self drawTickSet:context withOffset:offset + pixelsPerMM     withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:tickScaleFactor * 4];
        [self drawTickSet:context withOffset:offset + pixelsPerMM * 2 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:tickScaleFactor * 4];
        [self drawTickSet:context withOffset:offset + pixelsPerMM * 3 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:tickScaleFactor * 4];
        [self drawTickSet:context withOffset:offset + pixelsPerMM * 4 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:tickScaleFactor * 4];
        [self drawTickSet:context withOffset:offset + pixelsPerMM * 6 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:tickScaleFactor * 4];
        [self drawTickSet:context withOffset:offset + pixelsPerMM * 7 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:tickScaleFactor * 4];
        [self drawTickSet:context withOffset:offset + pixelsPerMM * 8 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:tickScaleFactor * 4];
        [self drawTickSet:context withOffset:offset + pixelsPerMM * 9 withTickCount:wholeCM withSpacing:pixelsPerCM withTickHeight:tickScaleFactor * 4];
    }
    
    CGContextSetStrokeColorWithColor(context, [color CGColor]);
    CGContextSetLineWidth(context, lineWidth);
    CGContextStrokePath(context);
}

- (void) drawTickSet:(CGContextRef)context withOffset:(CGFloat)offsetY withTickCount:(CGFloat)count withSpacing:(CGFloat)spacing withTickHeight:(CGFloat)height
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
