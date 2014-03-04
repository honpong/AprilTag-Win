//
//  VisualizationController.h
//  RC3DKSampleApp
//
//  Created by Brian on 3/3/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

#import <RC3DK/RC3DK.h>

#import "AVSessionManager.h"
#import "LocationManager.h"
#import "MotionManager.h"
#import "VideoManager.h"

@interface VisualizationController : GLKViewController <RCSensorFusionDelegate>

/**
 Represents the position of the camera relative to the scene
 */
typedef NS_ENUM(int, RCViewpoint) {
    RCViewpointTopDown = 0,
    RCViewpointSide,
    RCViewpointAnimating
};


/**
 Represents what types of features we should display.
 */
typedef NS_ENUM(int, RCFeatureFilter) {
    RCFeatureFilterShowAll = 0,
    RCFeatureFilterShowGood
};


- (IBAction)startButtonTapped:(id)sender;
- (IBAction)zoomInButtonTapped:(id)sender;
- (IBAction)zoomOutButtonTapped:(id)sender;
- (IBAction)changeViewButtonTapped:(id)sender;

@property (weak, nonatomic) IBOutlet UIButton *startStopButton;
@property (weak, nonatomic) IBOutlet UITextField *distanceText;
@property (weak, nonatomic) IBOutlet UILabel *statusLabel;

@end
