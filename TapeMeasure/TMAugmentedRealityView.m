//
//  TMAugmentedRealityView.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/12/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMAugmentedRealityView.h"

@implementation TMAugmentedRealityView
{
    TMCrosshairsLayerDelegate *crosshairsDelegate;
    CALayer *crosshairsLayer;
    TMFeaturesLayer* featuresLayer;
    
    NSMutableArray* pointsPool;
    struct corvis_feature_info features[FEATURE_COUNT];
    float videoScale;
    int videoFrameOffset;
}

- (id) initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self != nil)
    {
        [self initialize];
    }
    return self;
}

- (id) initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self != nil)
    {
        [self initialize];
    }
    return self;
}

- (void) initialize
{
    crosshairsDelegate = [TMCrosshairsLayerDelegate new];
    crosshairsLayer = [CALayer new];
    [crosshairsLayer setDelegate:crosshairsDelegate];
    crosshairsLayer.hidden = YES;
    crosshairsLayer.frame = self.frame;
    [crosshairsLayer setNeedsDisplay];
    [self.layer addSublayer:crosshairsLayer];
    
    [self setupFeatureDisplay];
}

- (void) setupFeatureDisplay
{
    featuresLayer = [[TMFeaturesLayer alloc] initWithFeatureCount:FEATURE_COUNT];
    featuresLayer.hidden = YES;
    featuresLayer.frame = self.frame;
    [featuresLayer setNeedsDisplay];
    [self.layer insertSublayer:featuresLayer below:crosshairsLayer];
    
    // create a pool of point objects to use in feature display
    pointsPool = [[NSMutableArray alloc] initWithCapacity:FEATURE_COUNT];
    for (int i = 0; i < FEATURE_COUNT; i++)
    {
        TMPoint* point = (TMPoint*)[DATA_MANAGER getNewObjectOfType:[TMPoint getEntity]];
        [pointsPool addObject:point];
    }
    
    // create the array of feature structs that we pass into corvis
    for (int i = 0; i < FEATURE_COUNT; i++)
    {
        struct corvis_feature_info newFeature;
        features[i] = newFeature;
    }
    
    // the scale of the video vs the video preview frame
    videoScale = (float)self.frame.size.width / (float)VIDEO_WIDTH;
    
    // videoFrameOffset is necessary to align the features properly. the video is being cropped to fit the view, which is slightly less tall than the video
    videoFrameOffset = (lrintf(VIDEO_HEIGHT * videoScale) - self.frame.size.height) / 2;
}

- (void) updateFeaturesWithX:(float)x withY:(float)y
{
    int count = [CORVIS_MANAGER getCurrentFeatures:features withMax:FEATURE_COUNT];
    NSMutableArray* trackedFeatures = [NSMutableArray arrayWithCapacity:count]; // the points we will display on screen
    for (int i = 0; i < count; i++)
    {
        TMPoint* point = [pointsPool objectAtIndex:i]; //get a point from the pool
        point.imageX = self.frame.size.width - lrintf(features[i].y * videoScale);
        point.imageY = lrintf(features[i].x * videoScale) - videoFrameOffset;
        point.quality = features[i].quality;
        [trackedFeatures addObject:point];
    }
    
    [featuresLayer setFeaturePositions:trackedFeatures];
    //    [featuresLayer setFeaturePositions:pointsPool]; //for testing
}

- (void) showCrosshairs
{
    crosshairsLayer.hidden = NO;
    [crosshairsLayer needsLayout];
}

- (void) hideCrosshairs
{
    crosshairsLayer.hidden = YES;
    [crosshairsLayer needsLayout];
}

- (void) showFeatures
{
    featuresLayer.hidden = NO;
}

- (void) hideFeatures
{
    featuresLayer.hidden = YES;
}

@end
