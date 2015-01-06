//
//  Created by Ben Hirashima on 12/29/14.
//  Copyright (c) 2014 Sunlayar. All rights reserved.
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
#import "SLConstants.h"
#import "RCVideoPreview.h"
#import "RC3DKPlus.h"

@interface SLCameraController : UIViewController <RCSensorFusionDelegate, RCStereoDelegate>

- (void)handlePause;
- (void)handleResume;
- (IBAction)handleShutterButton:(id)sender;

@property (nonatomic) IBOutlet RCVideoPreview *videoView;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;
@property (weak, nonatomic) IBOutlet UIView *uiContainer;
@property (weak, nonatomic) IBOutlet UIProgressView *progressBar;

@end
