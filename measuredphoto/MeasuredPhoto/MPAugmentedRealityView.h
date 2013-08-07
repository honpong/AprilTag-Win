//
//  TMAugmentedRealityView.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/12/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <RC3DK/RC3DK.h>
#import <RCCore/RCDistanceLabel.h>
#import "MPVideoPreview.h"
#import "MPLineLayer.h"
#import <RCCore/RCFeaturesLayer.h>

#define FEATURE_COUNT 200

@interface MPAugmentedRealityView : UIView

@property (weak, nonatomic) IBOutlet UIImageView *distanceBg;
@property (weak, nonatomic) IBOutlet RCDistanceLabel *distanceLabel;
@property (readonly) MPVideoPreview *videoView;
@property (readonly) RCFeaturesLayer* featuresLayer;
@property (readonly) RCFeaturesLayer* selectedFeaturesLayer;

- (void) initialize;
- (void) showFeatures;
- (void) hideFeatures;
- (RCFeaturePoint*) selectFeatureNearest:(CGPoint)coordinateTapped;
- (void) drawMeasurementBetweenPointA:(RCFeaturePoint*)pointA andPointB:(RCFeaturePoint*)pointB;
- (void) clearSelectedFeatures;

@end
