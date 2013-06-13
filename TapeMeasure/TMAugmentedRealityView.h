//
//  TMAugmentedRealityView.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/12/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMCrosshairsLayerDelegate.h"
#import "TMTickMarksLayerDelegate.h"
#import "TMFeatureLayerDelegate.h"
#import "TMFeaturesLayer.h"
#import "RCCore/feature_info.h"
#import "RCCore/RCDeviceInfo.h"
#import "RCCore/RCDistanceLabel.h"
#import "TMDataManagerFactory.h"
#import "RCCore/RCCorvisManagerFactory.h"
#import "TMVideoPreview.h"

#define FEATURE_COUNT 80
#define VIDEO_WIDTH 480
#define VIDEO_HEIGHT 640

@interface TMAugmentedRealityView : UIView

@property (weak, nonatomic) IBOutlet UIImageView *distanceBg;
@property (weak, nonatomic) IBOutlet RCDistanceLabel *distanceLabel;
@property TMVideoPreview *videoView;

- (void) showCrosshairs;
- (void) hideCrosshairs;
- (void) showFeatures;
- (void) hideFeatures;
- (void) updateFeaturesWithX:(float)x withY:(float)y;

@end
