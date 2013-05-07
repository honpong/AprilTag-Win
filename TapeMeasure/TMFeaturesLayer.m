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

- (id) initWithFeatureCount:(int)count
{
    self = [super init];
    if(self)
    {
        delegate = [[TMFeatureLayerDelegate alloc] init];
        int frameSize = (FEATURE_RADIUS * 2) + 4;
        
        featureCount = count;
        
        NSLog(@"Initializing %i feature layers", featureCount);
        
        for (int i = 0; i < featureCount; i++)
        {
            CALayer* newLayer = [CALayer new];
            newLayer.delegate = delegate;
            newLayer.hidden = YES;
            newLayer.frame = CGRectMake(0, 0, frameSize, frameSize);
            [newLayer setNeedsDisplay];
            [self addSublayer:newLayer];
        }
    }
    return self;
}

- (void) setFeaturePositions:(NSArray*)points
{
    int layerNum = 0;
    
    for (TMPoint* point in points)
    {
        CALayer* layer = [self.sublayers objectAtIndex:layerNum];
        float radius = layer.frame.size.height / 2;
        layer.frame = CGRectMake(point.imageX - radius, point.imageY - radius, layer.frame.size.width, layer.frame.size.height);
        layer.hidden = NO;
        [layer setNeedsLayout];
        layerNum++;
    }
    
    //hide any remaining unused layers
    for (int i = layerNum; i < featureCount; i++)
    {
        CALayer* layer = [self.sublayers objectAtIndex:layerNum];
        layer.hidden = YES;
    }
}

//turns off animations, reduces lag
- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}


@end
