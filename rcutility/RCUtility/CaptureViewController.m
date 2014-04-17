//
//  ViewController.m
//  RCCapture
//
//  Created by Brian on 9/27/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <RC3DK/RC3DK.h>
#import <RCCore/RCCore.h>
#import "CaptureViewController.h"
#import "AppDelegate.h"

@interface CaptureViewController ()
{
    bool isStarted;
    AVCaptureVideoPreviewLayer * previewLayer;
    RCCaptureManager * captureController;
}

@end

@implementation CaptureViewController

@synthesize previewView, startStopButton;

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.

	AVCaptureSession *session = [SESSION_MANAGER session];

	// Make a preview layer so we can see the visual output of an AVCaptureSession
	previewLayer = [[AVCaptureVideoPreviewLayer alloc] initWithSession:session];
	[previewLayer setVideoGravity:AVLayerVideoGravityResizeAspect];

    // add the preview layer to the hierarchy
    CALayer *rootLayer = [previewView layer];
	[rootLayer setBackgroundColor:[[UIColor blackColor] CGColor]];
	[rootLayer addSublayer:previewLayer];

    captureController = [[RCCaptureManager alloc] init];

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

- (void) captureDidStart
{
    [startStopButton setTitle:@"Stop" forState:UIControlStateNormal];
}

- (void) captureDidStop
{
    [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
    [startStopButton setEnabled:true];
    AppDelegate * app = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    [app startFromHome];
}

- (IBAction)startStopClicked:(id)sender
{
    if (!isStarted)
    {
        NSURL * fileurl = [AppDelegate timeStampedURLWithSuffix:@".capture"];
        [startStopButton setTitle:@"Starting..." forState:UIControlStateNormal];
        [captureController startCapture:fileurl.path withSession:[SESSION_MANAGER session] withDevice:[SESSION_MANAGER videoDevice] withDelegate:self];
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
