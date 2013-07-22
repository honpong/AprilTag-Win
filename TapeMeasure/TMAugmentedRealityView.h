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
#import <RC3DK/RC3DK.h>
#import "RCCore/RCDistanceLabel.h"
#import "TMVideoPreview.h"
#import "TMLineLayer.h"

#define FEATURE_COUNT 200

@interface TMAugmentedRealityView : UIView

@property (weak, nonatomic) IBOutlet UIImageView *distanceBg;
@property (weak, nonatomic) IBOutlet RCDistanceLabel *distanceLabel;
@property (readonly) TMVideoPreview *videoView;
@property (readonly) TMFeaturesLayer* featuresLayer;
@property (readonly) TMFeaturesLayer* selectedFeaturesLayer;

- (void) initialize;
- (void) showCrosshairs;
- (void) hideCrosshairs;
- (void) showFeatures;
- (void) hideFeatures;
- (TMPoint*) selectFeatureNearest:(CGPoint)coordinateTapped;
- (void) drawMeasurementBetweenPointA:(TMPoint*)pointA andPointB:(TMPoint*)pointB;
- (void) clearSelectedFeatures;

@end
