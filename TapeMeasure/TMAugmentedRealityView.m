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
    float videoScale;
    int videoFrameOffset;
    
    BOOL isInitialized;
}
@synthesize videoView;

- (void) initialize
{
    if (isInitialized) return;
    
    LOGME
    [TMOpenGLManagerFactory getInstance];
    
    videoView = [[TMVideoPreview alloc] initWithFrame:CGRectZero];
    [videoView setTransformFromCurrentVideoOrientationToOrientation:UIInterfaceOrientationPortrait];
    videoView.frame = CGRectMake(0, 0, self.frame.size.width, self.frame.size.height); // must be set AFTER setTransformFromCurrentVideoOrientationToOrientation
    [self addSubview:videoView];
    [self sendSubviewToBack:videoView];
    
    crosshairsDelegate = [TMCrosshairsLayerDelegate new];
    crosshairsLayer = [CALayer new];
    [crosshairsLayer setDelegate:crosshairsDelegate];
    crosshairsLayer.hidden = YES;
    crosshairsLayer.frame = self.frame;
    [crosshairsLayer setNeedsDisplay];
    [self.layer addSublayer:crosshairsLayer];
    
    [self setupFeatureDisplay];
        
    isInitialized = YES;
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
    
    // the scale of the video vs the video preview frame
    videoScale = (float)featuresLayer.frame.size.width / (float)VIDEO_WIDTH;
    
    // videoFrameOffset is necessary to align the features properly. the video is being cropped to fit the view, which is slightly less tall than the video
    videoFrameOffset = (lrintf(VIDEO_HEIGHT * videoScale) - featuresLayer.frame.size.height) / 2;
}

- (void) updateFeatures:(NSArray*)features
{
    NSMutableArray* trackedFeatures = [NSMutableArray arrayWithCapacity:features.count]; // the points we will display on screen
    
    for (int i = 0; i < features.count; i++)
    {
        RCFeaturePoint* feature = features[i];
        TMPoint* point = [pointsPool objectAtIndex:i]; //get a point from the pool
        point.imageX = featuresLayer.frame.size.width - rintf(feature.y * videoScale);
        point.imageY = rintf(feature.x * videoScale) - videoFrameOffset;
        point.quality = (1. - sqrt(feature.depth.standardDeviation/feature.depth.scalar));
        [trackedFeatures addObject:point];
    }
    
    [featuresLayer setFeaturePositions:trackedFeatures];
//    [featuresLayer setNeedsLayout];
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
