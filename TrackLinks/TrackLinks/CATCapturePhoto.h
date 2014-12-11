//
//  TMViewController.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
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
#import "CATConstants.h"

@interface CATCapturePhoto : UIViewController <RCSensorFusionDelegate, RCStereoDelegate, UIAlertViewDelegate>

- (void)handlePause;
- (void)handleResume;
- (IBAction)handleShutterButton:(id)sender;

@property (nonatomic) IBOutlet CATAugmentedRealityView *arView;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;
@property (weak, nonatomic) IBOutlet UIView *uiContainer;

@end
