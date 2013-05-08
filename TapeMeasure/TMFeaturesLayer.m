//
//  TMFeatureLayerManager.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/3/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMFeaturesLayer.h"

@implementation TMFeaturesLayer

int featureCount = 100; //default
TMFeatureLayerDelegate* delegate;
NSMutableArray* layersHighQ;
NSMutableArray* layersMediumQ;
NSMutableArray* layersLowQ;

- (id) initWithFeatureCount:(int)count
{
    self = [super init];
    if(self)
    {
        delegate = [[TMFeatureLayerDelegate alloc] init];
        int frameSize = (FEATURE_RADIUS * 2) + 4;
        
        featureCount = count;
        
        layersHighQ = [NSMutableArray arrayWithCapacity:featureCount];
        layersMediumQ = [NSMutableArray arrayWithCapacity:featureCount];
        layersLowQ = [NSMutableArray arrayWithCapacity:featureCount];
        
        NSLog(@"Initializing %i feature layers", featureCount);
        
        for (int i = 0; i < featureCount; i++)
        {
            CALayer* newLayer = [CALayer new];
            newLayer.delegate = delegate;
            newLayer.hidden = YES;
            newLayer.frame = CGRectMake(0, 0, frameSize, frameSize);
            newLayer.strokeColor = [TMFeaturesLayer getColorForFeatureQuality:FeatureQualityHigh];
            [newLayer setNeedsDisplay];
            [layersHighQ addObject:newLayer];
            [self addSublayer:newLayer];
        }
        
        for (int i = 0; i < featureCount; i++)
        {
            CALayer* newLayer = [CALayer new];
            newLayer.delegate = delegate;
            newLayer.hidden = YES;
            newLayer.frame = CGRectMake(0, 0, frameSize, frameSize);
            newLayer.strokeColor = [TMFeaturesLayer getColorForFeatureQuality:FeatureQualityMedium];
            [newLayer setNeedsDisplay];
            [layersMediumQ addObject:newLayer];
            [self addSublayer:newLayer];
        }
        
        for (int i = 0; i < featureCount; i++)
        {
            CALayer* newLayer = [CALayer new];
            newLayer.delegate = delegate;
            newLayer.hidden = YES;
            newLayer.frame = CGRectMake(0, 0, frameSize, frameSize);
            newLayer.strokeColor = [TMFeaturesLayer getColorForFeatureQuality:FeatureQualityLow];
            [newLayer setNeedsDisplay];
            [layersLowQ addObject:newLayer];
            [self addSublayer:newLayer];
        }
    }
    return self;
}

- (void) setFeaturePositions:(NSArray*)points
{
    int layerNumLowQ = 0;
    int layerNumMediumQ = 0;
    int layerNumHighQ = 0;
    
    for (TMPoint* point in points)
    {
        CALayer* layer;
                
        switch (point.quality)
        {
            case FeatureQualityLow:
            {
                layer = [layersLowQ objectAtIndex:layerNumLowQ];
                layerNumLowQ++;
                break;
            }
            case FeatureQualityMedium:
            {
                layer = [layersMediumQ objectAtIndex:layerNumMediumQ];
                layerNumMediumQ++;
                break;
            }
            default: //FeatureQualityHigh
            {
                layer = [layersHighQ objectAtIndex:layerNumHighQ];
                layerNumHighQ++;
                break;
            }
        }
        
        layer.hidden = NO;
        
        float radius = layer.frame.size.height / 2;
        CGRect newFrame = CGRectMake(point.imageX - radius, point.imageY - radius, layer.frame.size.width, layer.frame.size.height);
        
        if (!CGRectEqualToRect(layer.frame, newFrame))
        {
            layer.frame = newFrame;
            [layer setNeedsLayout];
        }
    }
    
    //hide any remaining unused layers
    for (int i = layerNumHighQ; i < featureCount; i++)
    {
        CALayer* layer = [layersHighQ objectAtIndex:layerNumHighQ];
        layer.hidden = YES;
    }
    for (int i = layerNumMediumQ; i < featureCount; i++)
    {
        CALayer* layer = [layersMediumQ objectAtIndex:layerNumMediumQ];
        layer.hidden = YES;
    }
    for (int i = layerNumLowQ; i < featureCount; i++)
    {
        CALayer* layer = [layersLowQ objectAtIndex:layerNumLowQ];
        layer.hidden = YES;
    }
}

//turns off animations, reduces lag
- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

+ (UIColor*)getColorForFeatureQuality:(FeatureQuality)quality
{
    switch (quality)
    {
        case FeatureQualityLow:
            return [UIColor colorWithRed:0 green:200 blue:255 alpha:0.1];
            
        case FeatureQualityMedium:
            return [UIColor colorWithRed:0 green:200 blue:255 alpha:0.4];
        
        default: //FeatureQualityHigh
            return [UIColor colorWithRed:0 green:200 blue:255 alpha:1];
    }
}

@end
