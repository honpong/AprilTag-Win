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
    CALayer* tickMarksLayer;
    TMTickMarksLayerDelegate* tickMarksDelegate;
    
    NSMutableArray* pointsPool;
    struct corvis_feature_info features[FEATURE_COUNT];
    float videoScale;
    int videoFrameOffset;
    float screenWidthIn;
    float screenWidthCM;
    float pixelsPerInch;
    float pixelsPerCM;
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
    screenWidthCM = [RCDeviceInfo getPhysicalScreenMetersX] * 100;
    pixelsPerCM = self.frame.size.width / screenWidthCM;
    screenWidthIn = [RCDeviceInfo getPhysicalScreenMetersX] * INCHES_PER_METER;
    pixelsPerInch = self.frame.size.width / screenWidthIn;
    
    float circleRadius = 40.;
    crosshairsDelegate = [[TMCrosshairsLayerDelegate alloc] initWithRadius:circleRadius];
    crosshairsLayer = [CALayer new];
    [crosshairsLayer setDelegate:crosshairsDelegate];
    crosshairsLayer.hidden = YES;
    crosshairsLayer.frame = self.frame;
    [crosshairsLayer setNeedsDisplay];
    [self.layer addSublayer:crosshairsLayer];
    
    featuresLayer = [[TMFeaturesLayer alloc] initWithFeatureCount:FEATURE_COUNT];
    featuresLayer.hidden = YES;
    featuresLayer.frame = self.frame;
    [featuresLayer setNeedsDisplay];
    [self.layer insertSublayer:featuresLayer below:crosshairsLayer];
    
//    [self setupFeatureDisplay];
}

//- (void) setupFeatureDisplay
//{
//    // create a pool of point objects to use in feature display
//    pointsPool = [[NSMutableArray alloc] initWithCapacity:FEATURE_COUNT];
//    for (int i = 0; i < FEATURE_COUNT; i++)
//    {
//        TMPoint* point = (TMPoint*)[DATA_MANAGER getNewObjectOfType:[TMPoint getEntity]];
//        [pointsPool addObject:point];
//    }
//    
//    // create the array of feature structs that we pass into corvis
//    for (int i = 0; i < FEATURE_COUNT; i++)
//    {
//        struct corvis_feature_info newFeature;
//        features[i] = newFeature;
//    }
//    
//    // the scale of the video vs the video preview frame
//    videoScale = (float)self.frame.size.width / (float)VIDEO_WIDTH;
//    
//    // videoFrameOffset is necessary to align the features properly. the video is being cropped to fit the view, which is slightly less tall than the video
//    videoFrameOffset = (lrintf(VIDEO_HEIGHT * videoScale) - self.frame.size.height) / 2;
//}
//
//- (void)setupTickMarksLayer
//{
//    [[NSUserDefaults standardUserDefaults] synchronize]; //in case user just changed default setting
//    
//    if (tickMarksLayer == nil)
//    {
//        tickMarksLayer = [CALayer new];
//    }
//    else
//    {
//        tickMarksLayer.delegate = nil;
//        tickMarksDelegate = nil;
//    }
//    
//    tickMarksDelegate = [[TMTickMarksLayerDelegate alloc] initWithWidthMeters:[RCDeviceInfo getPhysicalScreenMetersX] withUnits:(Units)[[NSUserDefaults standardUserDefaults] integerForKey:PREF_UNITS]];
//    [tickMarksLayer setDelegate:tickMarksDelegate];
//    tickMarksLayer.hidden = YES;
//    tickMarksLayer.frame = CGRectMake(self.view.frame.origin.x, self.view.frame.origin.y, self.view.frame.size.width * 2, self.view.frame.size.height);
//    [tickMarksLayer setNeedsDisplay];
//    [self.distanceBg.layer addSublayer:tickMarksLayer];
//}
@end
