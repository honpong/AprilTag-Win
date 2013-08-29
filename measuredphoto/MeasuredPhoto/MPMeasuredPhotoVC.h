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
#import "TestFlight.h"

@interface MPMeasuredPhotoVC : MPViewController <RCSensorFusionDelegate, UIAlertViewDelegate>

- (void)handlePause;
- (void)handleResume;
- (IBAction)handleShutterButton:(id)sender;
- (IBAction)handleThumbnail:(id)sender;

@property (nonatomic) IBOutlet MPAugmentedRealityView *arView;
@property (weak, nonatomic) IBOutlet UIButton *shutterButton;
@property (weak, nonatomic) IBOutlet UIButton *thumbnail;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;
@property (weak, nonatomic) IBOutlet UIView *toolbar;

@end
