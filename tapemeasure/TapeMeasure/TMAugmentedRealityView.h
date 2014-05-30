//
//  TMAugmentedRealityView.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/12/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMCrosshairsLayerDelegate.h"
#import "TMTickMarksLayerDelegate.h"
#import "TMDataManagerFactory.h"
#import "TMVideoPreview.h"
#import "TMLineLayer.h"
#import <RCCore/RCCore.h>

#define FEATURE_COUNT 200

@interface TMAugmentedRealityView : UIView

@property (weak, nonatomic) IBOutlet UIImageView *distanceBg;
@property (weak, nonatomic) IBOutlet RCDistanceLabel *distanceLabel;
@property (readonly) TMVideoPreview *videoView;
@property (readonly) RCFeaturesLayer* featuresLayer;

- (void) initialize;
- (void) showCrosshairs;
- (void) hideCrosshairs;
- (void) showFeatures;
- (void) hideFeatures;

@end
