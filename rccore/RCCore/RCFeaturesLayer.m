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
    float videoScale;
    int videoFrameOffset;
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

- (void) layoutSublayers
{
    [super layoutSublayers];
    
    // the scale of the video vs the video preview frame
    videoScale = (float)self.frame.size.width / (float)VIDEO_WIDTH;
    
    // videoFrameOffset is necessary to align the features properly. the video is being cropped to fit the view, which is slightly less tall than the video
    videoFrameOffset = (lrintf(VIDEO_HEIGHT * videoScale) - self.frame.size.height) / 2;
}

- (void) updateFeatures:(NSArray*)features // An array of RCFeaturePoint objects
{
    trackedPoints = features;
    
    int layerNum = 0;
    
    for (RCFeaturePoint* feature in features)
    {
        if(feature.initialized)
        {
            CALayer* layer = [self.sublayers objectAtIndex:layerNum];
            layer.hidden = NO;
            
            float quality = (1. - sqrt(feature.depth.standardDeviation/feature.depth.scalar));
            layer.opacity = quality > 0.2 ? quality : 0.2;
            
            layer.position = [self screenPointFromFeature:feature];
            //        layer.position = CGPointMake(screenPoint.x, screenPoint.y - self.frame.origin.y); // is this necessary anymore?
            
            [layer setNeedsLayout];
            layerNum++;
        }
    }
    
    //hide any remaining unused layers
    for (int i = layerNum; i < featureCount; i++)
    {
        CALayer* layer = [self.sublayers objectAtIndex:i];
        if(!layer.hidden)
        {
            layer.hidden = YES;
            [layer setNeedsLayout];
        }
    }
}

- (RCFeaturePoint*) getClosestPointTo:(CGPoint)tappedPoint
{
    CGPoint cameraPoint = [self cameraPointFromScreenPoint:tappedPoint];
    
    RCFeaturePoint* closestPoint = nil;
    float closestPointDist = 50.;
    
    for (RCFeaturePoint* thisPoint in trackedPoints)
    {
        if (thisPoint.initialized)
        {
            float dist = [thisPoint pixelDistanceToPoint:cameraPoint];
            if (dist < closestPointDist)
            {
                closestPointDist = dist;
                closestPoint = thisPoint;
            }
        }
    }
    
    return closestPoint;
}

- (CGPoint) screenPointFromFeature:(RCFeaturePoint*)feature
{
    float x = self.frame.size.width - (feature.y * videoScale);
    float y = (feature.x * videoScale) - videoFrameOffset;
    return CGPointMake(x, y);
}

- (CGPoint) cameraPointFromScreenPoint:(CGPoint)screenPoint
{
    float x = screenPoint.y / videoScale;
    float y = (self.frame.size.width - screenPoint.x) / videoScale;
    return CGPointMake(x, y);
}

//turns off animations, reduces lag
- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end