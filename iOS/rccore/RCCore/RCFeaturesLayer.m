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
    float videoScale;
    int videoFrameOffsetX, videoFrameOffsetY;
}
@synthesize features;

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
            newLayer.bounds = CGRectMake(0, 0, kMPFeatureFrameSize, kMPFeatureFrameSize);
            newLayer.position = self.position;
            newLayer.contentsScale = 2.;
            [newLayer setNeedsDisplay];
            [self addSublayer:newLayer];
        }
    }
    return self;
}

- (void) layoutSublayers
{
    [super layoutSublayers];
    
    // calculate the scale of the video vs the video preview frame
    float videoAspect = VIDEO_HEIGHT / VIDEO_WIDTH;
    float frameAspect = self.bounds.size.height / self.bounds.size.width;
    if (videoAspect > frameAspect)
    {
        videoScale = (float)self.bounds.size.width / (float)VIDEO_WIDTH;
    }
    else
    {
        videoScale = (float)self.bounds.size.height / (float)VIDEO_HEIGHT;
    }
    
    // necessary to align the features properly. the video is being cropped to fit the view.
    videoFrameOffsetX = ((VIDEO_WIDTH * videoScale) - self.bounds.size.width) / 2;
    videoFrameOffsetY = ((VIDEO_HEIGHT * videoScale) - self.bounds.size.height) / 2;
}

- (void) updateFeatures:(NSArray*)features_ // An array of RCFeaturePoint objects
{
    features = features_;
    
    int layerNum = 0;
    
    for (RCFeaturePoint* feature in features_)
    {
        CALayer* layer = (self.sublayers)[layerNum];
        layer.hidden = NO;
//        float quality = (1. - sqrt(feature.originalDepth.standardDeviation/feature.originalDepth.scalar));
        layer.opacity = 1.; //quality > 0.2 ? quality : 0.2;
        layer.position = [self screenPointFromFeature:feature];
        [layer setNeedsLayout];
        layerNum++;
    }
    
    //hide any remaining unused layers
    for (int i = layerNum; i < featureCount; i++)
    {
        CALayer* layer = (self.sublayers)[i];
        if(!layer.hidden)
        {
            layer.hidden = YES;
            [layer setNeedsLayout];
        }
    }
}

- (RCFeaturePoint*)getClosestFeatureTo:(CGPoint)tappedPoint
{
    CGPoint cameraPoint = [self cameraPointFromScreenPoint:tappedPoint];
    
    RCFeaturePoint* closestPoint = nil;
    float closestPointDist = 50.;
    
    for (RCFeaturePoint* thisPoint in features)
    {
        float dist = [thisPoint pixelDistanceToPoint:cameraPoint];
        if (dist < closestPointDist)
        {
            closestPointDist = dist;
            closestPoint = thisPoint;
        }
    }
    
    return closestPoint;
}

- (CGPoint) screenPointFromFeature:(RCFeaturePoint*)feature
{
    CGFloat x = self.bounds.size.width - (feature.y * videoScale) + videoFrameOffsetX;
    CGFloat y = (feature.x * videoScale) - videoFrameOffsetY;
    return CGPointMake(x, y);
}

- (CGPoint) cameraPointFromScreenPoint:(CGPoint)screenPoint
{
    CGFloat x = (screenPoint.y + videoFrameOffsetY)/ videoScale;
    CGFloat y = ((self.bounds.size.width - screenPoint.x  + videoFrameOffsetX) / videoScale);
    return CGPointMake(x, y);
}

//turns off animations, reduces lag
- (id<CAAction>)actionForLayer:(CALayer *)layer forKey:(NSString *)event
{
    return (id)[NSNull null];
}

@end