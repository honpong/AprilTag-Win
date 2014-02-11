//
//  TMAugmentedRealityView.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/12/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <RCCore/RCCore.h>
#import "MPVideoPreview.h"
#import "MPLineLayer.h"
#import "MPMeasurementsView.h"
#import "ACMagnifyingView.h"
#import "MPImageView.h"
#import "MPRotatingView.h"
#import "MPInstructionsView.h"

#define FEATURE_COUNT 200

@interface MPAugmentedRealityView : ACMagnifyingView <MPRotatingView>

@property (weak, nonatomic) IBOutlet UIImageView *distanceBg;
@property (weak, nonatomic) IBOutlet RCDistanceLabel *distanceLabel;
@property (readonly) MPVideoPreview *videoView;
@property (readonly) RCFeaturesLayer* featuresLayer;
@property (readonly) RCFeaturesLayer* selectedFeaturesLayer;
@property (readonly) RCFeaturesLayer* initializingFeaturesLayer;
@property (readonly) MPMeasurementsView* measurementsView;
@property (readonly) UIView* featuresView;
@property (readonly) MPImageView* photoView;
@property (nonatomic, getter = isMagGlassEnabled) BOOL magGlassEnabled;
@property (readonly, nonatomic) MPInstructionsView* instructionsView;

- (void) initialize;
- (void) showFeatures;
- (void) hideFeatures;
- (RCFeaturePoint*) selectFeatureNearest:(CGPoint)coordinateTapped;
- (void) selectFeature:(RCFeaturePoint*)point;
- (void) clearSelectedFeatures;
- (void) handleFeatureTapped:(CGPoint)coordinateTapped;
- (void) resetSelectedFeatures;

@end
