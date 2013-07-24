//
//  RCFeaturesLayer.m
//  RCCore
//
//  Created by Ben Hirashima on 7/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCFeaturesLayer.h"

@implementation RCFeaturesLayer
{
    int featureCount;
    RCFeatureLayerDelegate* delegate;
    NSArray* trackedPoints;
}

- (id) initWithFeatureCount:(int)count andColor:(UIColor*)featureColor
{
    if(self = [super init])
    {
        featureCount = count;
        DLog(@"Initializing %i feature layers", featureCount);
        
        delegate = [[RCFeatureLayerDelegate alloc] init];
        if (featureColor) delegate.color = featureColor;
        
        for (int i = 0; i < count; i++)
        {
            CALayer* newLayer = [CALayer new];
            newLayer.delegate = delegate;
            newLayer.hidden = YES;
            newLayer.bounds = CGRectMake(0, 0, FRAME_SIZE, FRAME_SIZE);
            newLayer.position = self.position;
            [newLayer setNeedsDisplay];
            [self addSublayer:newLayer];
        }
    }
    return self;
}

- (void) updateFeatures:(NSArray*)features // An array of RCFeaturePoint objects
{
    trackedPoints = features;
    
    // the scale of the video vs the video preview frame
    float videoScale = (float)self.frame.size.width / (float)VIDEO_WIDTH;
    
    // videoFrameOffset is necessary to align the features properly. the video is being cropped to fit the view, which is slightly less tall than the video
    int videoFrameOffset = (lrintf(VIDEO_HEIGHT * videoScale) - self.frame.size.height) / 2;
    
    int layerNum = 0;
    
    for (RCFeaturePoint* point in features)
    {
        CALayer* layer = [self.sublayers objectAtIndex:layerNum];
        layer.hidden = NO;
        
        float quality = (1. - sqrt(point.depth.standardDeviation/point.depth.scalar));
        layer.opacity = quality > 0.2 ? quality : 0.2;
        
        float x = self.frame.size.width - rintf(point.y * videoScale);
        float y = rintf(point.x * videoScale) - videoFrameOffset;
        layer.position = CGPointMake(x, y - self.frame.origin.y);
        
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

- (RCFeaturePoint*) getClosestPointTo:(CGPoint)searchPoint
{
    RCFeaturePoint* closestPoint;
    float closestPointDist = 1000000.;
    
    for (RCFeaturePoint* thisPoint in trackedPoints)
    {
        if (closestPoint)
        {
            float dist = [thisPoint pixelDistanceToPoint:searchPoint];
            if (dist < closestPointDist)
            {
                closestPointDist = dist;
                closestPoint = thisPoint;
            }
        }
        else
        {
            closestPoint = thisPoint;
            closestPointDist = [thisPoint pixelDistanceToPoint:searchPoint];
        }
    }
    
    return closestPoint;
}

//turns off animations, reduces lag
- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end