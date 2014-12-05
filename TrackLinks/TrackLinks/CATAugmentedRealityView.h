//
//  TMAugmentedRealityView.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/12/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "CATInstructionsView.h"
#import "RCVideoPreview.h"
#import <RC3DK/RC3DK.h>
#import "CATFeaturesLayer.h"

#define FEATURE_COUNT 200

@interface CATAugmentedRealityView : UIView

@property (weak, nonatomic) IBOutlet UIImageView *distanceBg;
//@property (weak, nonatomic) IBOutlet RCDistanceLabel *distanceLabel;
@property (readonly) RCVideoPreview *videoView;
@property (readonly) CATFeaturesLayer* featuresLayer;
@property (readonly) CATFeaturesLayer* initializingFeaturesLayer;
@property (readonly) UIView* featuresView;

- (void) initialize;
- (void) showFeatures;
- (void) hideFeatures;
- (RCFeaturePoint*) selectFeatureNearest:(CGPoint)coordinateTapped;
- (void) selectFeature:(RCFeaturePoint*)point;
- (void) clearSelectedFeatures;
- (void) handleFeatureTapped:(CGPoint)coordinateTapped;
- (void) resetSelectedFeatures;

@end
