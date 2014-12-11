//
//  TMAugmentedRealityView.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/12/13.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import "RCVideoPreview.h"
#import "RC3DK.h"
#import "CATFeaturesLayer.h"
#import "CATARDelegate.h"

#define FEATURE_COUNT 200

@interface CATAugmentedRealityView : UIView

@property (weak, nonatomic) IBOutlet UIImageView *distanceBg;
@property (readonly) RCVideoPreview *videoView;
@property (readonly) CATFeaturesLayer* featuresLayer;
@property (readonly) CATFeaturesLayer* initializingFeaturesLayer;
@property (readonly) UIView* featuresView;
@property (readonly) CATARDelegate* AROverlay;

- (void) initialize;
- (void) showFeatures;
- (void) hideFeatures;

@end
