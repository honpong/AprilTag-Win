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
#import <RCCore/RCCore.h>
#import "MBProgressHUD.h"
#import "MPVideoPreview.h"
#import "MPAugmentedRealityView.h"
#import "MPViewController.h"
#import "MPAnalytics.h"
#import "MPSlideBanner.h"
#import "MPToolbarView.h"
#import "MPThumbnailButton.h"
#import "MPShutterButton.h"
#import "MPContainerView.h"

@interface MPMeasuredPhotoVC : MPViewController <RCSensorFusionDelegate, UIAlertViewDelegate, MPAugRealityViewDelegate, MPInstructionsViewDelegate>

- (void)handlePause;
- (void)handleResume;
- (IBAction)handleShutterButton:(id)sender;
- (IBAction)handleThumbnail:(id)sender;
- (IBAction)handleQuestionButton:(id)sender;
- (IBAction)handleQuestionCloseButton:(id)sender;

@property (nonatomic) IBOutlet MPAugmentedRealityView *arView;
@property (weak, nonatomic) IBOutlet MPShutterButton *shutterButton;
@property (weak, nonatomic) IBOutlet MPThumbnailButton *thumbnail;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;
@property (weak, nonatomic) IBOutlet MPToolbarView *toolbar;
@property (weak, nonatomic) IBOutlet UILabel *questionLabel;
@property (weak, nonatomic) IBOutlet UISegmentedControl *questionSegButton;
@property (weak, nonatomic) IBOutlet MPSlideBanner *questionView;
@property (weak, nonatomic) IBOutlet UIButton *questionCloseButton;
@property (weak, nonatomic) IBOutlet MPContainerView *containerView;
@property (weak, nonatomic) IBOutlet MPInstructionsView *instructionsView;

@end
