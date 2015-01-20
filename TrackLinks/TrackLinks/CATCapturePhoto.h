
//  Copyright (c) 2014 Caterpillar. All rights reserved.


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
#import "CATConstants.h"
#import "RCVideoPreview.h"
#import "RC3DKPlus.h"

@interface CATCapturePhoto : UIViewController <RCSensorFusionDelegate, RCStereoDelegate>

- (void)handlePause;
- (void)handleResume;
- (IBAction)handleShutterButton:(id)sender;

@property (nonatomic) IBOutlet RCVideoPreview *videoView;
@property (weak, nonatomic) IBOutlet UILabel *messageLabel;
@property (weak, nonatomic) IBOutlet UIView *uiContainer;
@property (weak, nonatomic) IBOutlet UIProgressView *progressBar;

@end
