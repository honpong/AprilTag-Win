//
//  VisualizationController.h
//  RC3DKSampleApp
//
//  Created by Brian on 3/3/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>
#import "RCAVSessionManager.h"
#import "RCLocationManager.h"
#import "RCMotionManager.h"
#import "RCVideoManager.h"

@interface VisualizationController : GLKViewController <RCSensorFusionDelegate, UIGestureRecognizerDelegate>

/**
 Represents the position of the camera relative to the scene
 */
typedef NS_ENUM(int, RCViewpoint) {
    RCViewpointManual = 0,
    RCViewpointAnimating,
    RCViewpointTopDown,
    RCViewpointSide
};

/**
 Represents what types of features we should display.
 */
typedef NS_ENUM(int, RCFeatureFilter) {
    RCFeatureFilterShowAll = 0,
    RCFeatureFilterShowGood
};

- (IBAction)startButtonTapped:(id)sender;
- (IBAction)changeViewButtonTapped:(id)sender;
- (IBAction)handlePanGesture:(UIPanGestureRecognizer*)sender;
- (IBAction)handlePinchGesture:(UIPinchGestureRecognizer*)sender;
- (IBAction)handleRotationGesture:(UIRotationGestureRecognizer*)sender;

@property (weak, nonatomic) IBOutlet UIButton *startStopButton;
@property (weak, nonatomic) IBOutlet UILabel *statusLabel;
@property (strong, nonatomic) IBOutlet UIPinchGestureRecognizer *pinchRecognizer;
@property (strong, nonatomic) IBOutlet UIPanGestureRecognizer *panRecognizer;
@property (strong, nonatomic) IBOutlet UIRotationGestureRecognizer *rotationRecognizer;

@end
