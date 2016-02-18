//
//  TM2DTape.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/13/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TM2DTapeView.h"

@implementation TM2DTapeView
{
    CALayer* tickMarksLayer;
    TMTickMarksLayerDelegate* tickMarksDelegate;
    
    float screenWidthIn;
    float screenWidthCM;
    float pixelsPerInch;
    float pixelsPerCM;
    
    Units measurementUnits;
    
    BOOL isInitialized;
}

- (void)drawTickMarksWithUnits:(Units)units
{
    if (isInitialized) return;
    
    screenWidthCM = [RCDeviceInfo getPhysicalScreenMetersX] * 100;
    pixelsPerCM = self.frame.size.width / screenWidthCM;
    screenWidthIn = [RCDeviceInfo getPhysicalScreenMetersX] * INCHES_PER_METER;
    pixelsPerInch = self.frame.size.width / screenWidthIn;
    
    measurementUnits = units;
        
    if (tickMarksLayer == nil)
    {
        tickMarksLayer = [CALayer new];
    }
    else
    {
        tickMarksLayer.delegate = nil;
        tickMarksDelegate = nil;
    }
    
    tickMarksDelegate = [[TMTickMarksLayerDelegate alloc] initWithWidthMeters:[RCDeviceInfo getPhysicalScreenMetersX] withUnits:measurementUnits];
    [tickMarksLayer setDelegate:tickMarksDelegate];
    tickMarksLayer.hidden = NO;
    tickMarksLayer.frame = CGRectMake(0, 0, self.frame.size.width * 2, self.frame.size.height);
    [tickMarksLayer setNeedsDisplay];
    [self.layer addSublayer:tickMarksLayer];
    [self setNeedsLayout];
        
    isInitialized = YES;
}

- (void)moveTapeWithDistance:(float)meters withUnits:(Units)units
{
    float xOffset = 0;
    
    meters = meters / 10; // slow it down
    
    if (units == UnitsImperial)
    {
        float inches = meters * INCHES_PER_METER;
        float distRemainder = inches - floor(inches);
        xOffset = distRemainder * pixelsPerInch;
        xOffset = xOffset - pixelsPerInch;
    }
    else
    {
        float centimeters = meters * 100;
        float distRemainder = centimeters - floor(centimeters);
        xOffset = distRemainder * pixelsPerCM;
        xOffset = xOffset - pixelsPerCM;
    }
    
    tickMarksLayer.frame = CGRectMake(xOffset, tickMarksLayer.frame.origin.y, tickMarksLayer.frame.size.width, tickMarksLayer.frame.size.height);
    [tickMarksLayer needsLayout];
}

@end
