//
//  TMViewController.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <GLKit/GLKit.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreImage/CoreImage.h>
#import <ImageIO/ImageIO.h>
#import <CoreGraphics/CoreGraphics.h>
#import <CoreLocation/CoreLocation.h>
#import <RC3DK/RC3DK.h>
#import "MBProgressHUD.h"
#import "MPVideoPreview.h"
#import "RCCore/RCDistanceLabel.h"
#import "MPAugmentedRealityView.h"
#import "RCCore/RCDistanceMetric.h"
#import "MPViewController.h"
#import "MPAnalytics.h"

@interface MPMeasuredPhotoVC : MPViewController <RCSensorFusionDelegate>

- (void)handlePause;
- (void)handleResume;
- (IBAction)handleSaveButton:(id)sender;
- (void)startMeasuring;
- (void)stopMeasuring;

@property (weak, nonatomic) IBOutlet UIImageView *statusIcon;
@property (weak, nonatomic) IBOutlet RCDistanceLabel *distanceLabel;
@property (weak, nonatomic) IBOutlet UILabel *lblInstructions;
@property (nonatomic) IBOutlet MPAugmentedRealityView *arView;
@property (weak, nonatomic) IBOutlet UIView *instructionsBg;
@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnSave;

@end
