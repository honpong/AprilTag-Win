//
//  TMViewController.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 Caterpillar. All rights reserved.
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
#import "MBProgressHUD.h"
#import "CATAugmentedRealityView.h"
#import "CATToolbarView.h"
#import "CATGalleryButton.h"
#import "CATShutterButton.h"
#import "CATOrientationChangeData.h"
#import "CATContainerView.h"
#import "CATConstants.h"
#import "MPExpandingCircleAnimationView.h"

@interface CATCapturePhoto : UIViewController <RCSensorFusionDelegate, RCStereoDelegate, UIAlertViewDelegate>

+ (UIDeviceOrientation) getCurrentUIOrientation;
- (void)handlePause;
- (void)handleResume;
- (IBAction)handleShutterButton:(id)sender;
- (void) setOrientation:(UIDeviceOrientation)orientation animated:(BOOL)animated;

@property (nonatomic) IBOutlet CATAugmentedRealityView *arView;
@property (weak, nonatomic) IBOutlet CATShutterButton *shutterButton;
@property (weak, nonatomic) IBOutlet CATGalleryButton *galleryButton;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;
@property (weak, nonatomic) IBOutlet CATToolbarView *toolbar;
@property (weak, nonatomic) IBOutlet CATContainerView *containerView;
@property (weak, nonatomic) IBOutlet MPExpandingCircleAnimationView *expandingCircleView;

@end
