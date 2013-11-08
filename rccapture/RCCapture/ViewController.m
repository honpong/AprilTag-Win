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
#import "AVSessionManager.h"
#import "VideoManager.h"

@interface ViewController ()
{
    bool isStarted;
    AVCaptureVideoPreviewLayer * previewLayer;
    CaptureController * captureController;
}

@end

@implementation ViewController

@synthesize previewView, startStopButton;

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.

	AVCaptureSession *session = [[AVSessionManager sharedInstance] session];

	// Make a preview layer so we can see the visual output of an AVCaptureSession
	previewLayer = [[AVCaptureVideoPreviewLayer alloc] initWithSession:session];
	[previewLayer setVideoGravity:AVLayerVideoGravityResizeAspect];

    // add the preview layer to the hierarchy
    CALayer *rootLayer = [previewView layer];
	[rootLayer setBackgroundColor:[[UIColor blackColor] CGColor]];
	[rootLayer addSublayer:previewLayer];

    captureController = [[CaptureController alloc] init];

    isStarted = false;

    // start the capture session running, note this is an async operation
    // status is provided via notifications such as AVCaptureSessionDidStartRunningNotification/AVCaptureSessionDidStopRunningNotification
    [session startRunning];
}

- (void) viewDidLayoutSubviews
{
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


- (void) captureDidStop
{
    NSLog(@"did finish");
    [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
    [startStopButton setEnabled:true];
    [[AVSessionManager sharedInstance] unlockFocus];
    AppDelegate * app = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    [app startFromCalibration];
}

- (IBAction)startStopClicked:(id)sender
{
    if (!isStarted)
    {
        NSLog(@"Starting");
        NSURL * fileurl = [AppDelegate timeStampedURLWithSuffix:@".capture"];
        [[AVSessionManager sharedInstance] lockFocus];
        [captureController startCapture:fileurl.path withSession:[[AVSessionManager sharedInstance] session] withDelegate:self];

        [startStopButton setTitle:@"Stop" forState:UIControlStateNormal];
    }
    else
    {
        [startStopButton setEnabled:false];
        [startStopButton setTitle:@"Stopping..." forState:UIControlStateNormal];
        [captureController stopCapture];
    }
    isStarted = !isStarted;
}

@end
