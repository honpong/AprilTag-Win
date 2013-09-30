//
//  ViewController.m
//  RCCapture
//
//  Created by Brian on 9/27/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <RC3DK/RC3DK.h>
#import <RCCore/RCCore.h>
#import "ViewController.h"
#import "AppDelegate.h"

@interface ViewController ()
{
    bool isStarted;
    AVCaptureVideoPreviewLayer * previewLayer;
}

@end

@implementation ViewController

@synthesize previewView, startStopButton;

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.

	AVCaptureSession *session = [[RCAVSessionManager sharedInstance] session];
    [[RCVideoManager sharedInstance] setupWithSession:session];

	// Make a preview layer so we can see the visual output of an AVCaptureSession
	previewLayer = [[AVCaptureVideoPreviewLayer alloc] initWithSession:session];
	[previewLayer setVideoGravity:AVLayerVideoGravityResizeAspect];

    // add the preview layer to the hierarchy
    CALayer *rootLayer = [previewView layer];
	[rootLayer setBackgroundColor:[[UIColor blackColor] CGColor]];
	[rootLayer addSublayer:previewLayer];

    isStarted = false;

    // start the capture session running, note this is an async operation
    // status is provided via notifications such as AVCaptureSessionDidStartRunningNotification/AVCaptureSessionDidStopRunningNotification
    [session startRunning];
}

- (void) viewDidLayoutSubviews
{
    NSLog(@"Will appear bounds %f %f %f %f", [previewView bounds].origin.x,
          [previewView bounds].origin.y,
          [previewView bounds].size.width,
          [previewView bounds].size.height);
    NSLog(@"Layer %f %f %f %f", [previewLayer bounds].origin.x,
          [previewLayer bounds].origin.y,
          [previewLayer bounds].size.width,
          [previewLayer bounds].size.height);

    [self layoutPreviewInView:previewView];
}

-(void)layoutPreviewInView:(UIView *) aView
{
    AVCaptureVideoPreviewLayer *layer = previewLayer;
    if (!layer) return;

    UIDeviceOrientation orientation = [UIDevice currentDevice].orientation;
    CATransform3D transform = CATransform3DIdentity;
    if (orientation == UIDeviceOrientationPortrait) ;
    else if (orientation == UIDeviceOrientationLandscapeLeft)
        transform = CATransform3DMakeRotation(-M_PI_2, 0.0f, 0.0f, 1.0f);
    else if (orientation == UIDeviceOrientationLandscapeRight)
        transform = CATransform3DMakeRotation(M_PI_2, 0.0f, 0.0f, 1.0f);
    else if (orientation == UIDeviceOrientationPortraitUpsideDown)
        transform = CATransform3DMakeRotation(M_PI, 0.0f, 0.0f, 1.0f);

    previewLayer.transform = transform;
    previewLayer.frame = aView.bounds;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


- (IBAction)startStopClicked:(id)sender
{
    if (!isStarted)
    {
        NSLog(@"Starting");
        NSURL * fileurl = [AppDelegate timeStampedURLWithSuffix:@".capture"];
        [[RCSensorFusion sharedInstance] startCapture:fileurl.path];
        [[RCMotionManager sharedInstance] startMotionCapture];
        [[RCVideoManager sharedInstance] startVideoCapture];

        [startStopButton setTitle:@"Stop" forState:UIControlStateNormal];
    }
    else
    {
        [[RCVideoManager sharedInstance] stopVideoCapture];
        [[RCMotionManager sharedInstance] stopMotionCapture];
        [[RCSensorFusion sharedInstance] stopCapture];
        [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
        AppDelegate * app = (AppDelegate *)[[UIApplication sharedApplication] delegate];
        [app startFromCalibration];
    }
    isStarted = !isStarted;
}

@end
