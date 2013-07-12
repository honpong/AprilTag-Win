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
    NSMutableArray* pointsPool;
    NSMutableArray* trackedPoints;
}

- (id) initWithFeatureCount:(int)count andColor:(UIColor*)featureColor
{
    if(self = [super init])
    {
        featureCount = count;
        NSLog(@"Initializing %i feature layers", featureCount);
        
        delegate = [[TMFeatureLayerDelegate alloc] init];
        if (featureColor) delegate.color = featureColor;
        
        for (int i = 0; i < count; i++)
        {
            CALayer* newLayer = [CALayer new];
            newLayer.delegate = delegate;
            newLayer.hidden = YES;
            newLayer.frame = CGRectMake(0, 0, FRAME_SIZE, FRAME_SIZE);
            [newLayer setNeedsDisplay];
            [self addSublayer:newLayer];
        }
        
        [self createPointsPool];
    }
    return self;
}

- (void) createPointsPool
{
    // create a pool of point objects to use in feature display
    pointsPool = [[NSMutableArray alloc] initWithCapacity:featureCount];
    for (int i = 0; i < featureCount; i++)
    {
        TMPoint* point = (TMPoint*)[DATA_MANAGER getNewObjectOfType:[TMPoint getEntity]];
        [pointsPool addObject:point];
    }
}

- (void) updateFeatures:(NSArray*)features // An array of RCFeaturePoint objects 
{
    // the scale of the video vs the video preview frame
    float videoScale = (float)self.frame.size.width / (float)VIDEO_WIDTH;
    
    // videoFrameOffset is necessary to align the features properly. the video is being cropped to fit the view, which is slightly less tall than the video
    int videoFrameOffset = (lrintf(VIDEO_HEIGHT * videoScale) - self.frame.size.height) / 2;
    
    trackedPoints = [NSMutableArray arrayWithCapacity:features.count]; // the points we will display on screen
    
    for (int i = 0; i < features.count; i++)
    {
        RCFeaturePoint* feature = features[i];
        if(feature.initialized)
        {
            TMPoint* point = [pointsPool objectAtIndex:i]; //get a point from the pool
            point.imageX = self.frame.size.width - rintf(feature.y * videoScale);
            point.imageY = rintf(feature.x * videoScale) - videoFrameOffset;
            point.quality = (1. - sqrt(feature.depth.standardDeviation/feature.depth.scalar));
            point.feature = feature;
            [trackedPoints addObject:point];
        }
    }

    [self setFeaturePositions:trackedPoints];
}

- (void) setFeaturePositions:(NSArray*)points // An array of TMPoint objects 
{
    int layerNum = 0;
    
    for (TMPoint* point in points)
    {
        int x = point.imageX - FRAME_RADIUS;
        int y = point.imageY - FRAME_RADIUS;
        
        CALayer* layer = [self.sublayers objectAtIndex:layerNum];
        layer.hidden = NO;
        layer.opacity = point.quality > 0.2 ? point.quality : 0.2;
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

- (TMPoint*) getClosestPointTo:(CGPoint)searchPoint
{
    TMPoint* closestPoint;
    float closestPointDist = 1000000.;
    
    for (TMPoint* thisPoint in trackedPoints)
    {
        if (closestPoint)
        {
            float dist = [thisPoint distanceToPoint:searchPoint];
            if (dist < closestPointDist)
            {
                closestPointDist = dist;
                closestPoint = thisPoint;
            }
        }
        else
        {
            closestPoint = thisPoint;
            closestPointDist = [thisPoint distanceToPoint:searchPoint];
        }
    }
    
    closestPoint.quality = 1.;
    return closestPoint;
}

//turns off animations, reduces lag
- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end
