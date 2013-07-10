//
//  TMFeatureLayerManager.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 5/3/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMFeaturesLayer.h"

@implementation TMFeaturesLayer
{
    int featureCount; 
    TMFeatureLayerDelegate* delegate;
}

- (id) initWithFeatureCount:(int)count
{
    self = [super init];
    if(self)
    {
        delegate = [[TMFeatureLayerDelegate alloc] init];
        
        featureCount = count;
        NSLog(@"Initializing %i feature layers", featureCount);
        
        for (int i = 0; i < featureCount; i++)
        {
            CALayer* newLayer = [CALayer new];
            newLayer.delegate = delegate;
            newLayer.hidden = YES;
            newLayer.frame = CGRectMake(0, 0, FRAME_SIZE, FRAME_SIZE);
            [newLayer setNeedsDisplay];
            [self addSublayer:newLayer];
        }
    }
    return self;
}

- (void) setFeaturePositions:(NSArray*)points
{
    int layerNum = 0;
    float radius = FRAME_SIZE / 2;
    
    for (TMPoint* point in points)
    {
        CALayer* layer = [self.sublayers objectAtIndex:layerNum];
        layer.hidden = NO;
        layer.opacity = point.quality > 0.2 ? point.quality : 0.2;
        int x = point.imageX - radius;
        int y = point.imageY - radius;
        layer.frame = CGRectMake(x, y, layer.frame.size.width, layer.frame.size.height);
        [layer setNeedsLayout];
        
        layerNum++;
    }
    
    //hide any remaining unused layers
    for (int i = layerNum; i < featureCount; i++)
    {
        CALayer* layer = [self.sublayers objectAtIndex:i];
        layer.hidden = YES;
    }
}

//turns off animations, reduces lag
- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end
