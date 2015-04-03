//
//  ViewController.m
//  RCCapture
//
//  Created by Brian on 9/27/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

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

@synthesize previewView, frameRateSelector, resolutionSelector, startStopButton;

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.

	AVCaptureSession *session = [[RCAVSessionManager sharedInstance] session];

	// Make a preview layer so we can see the visual output of an AVCaptureSession
	previewLayer = [[AVCaptureVideoPreviewLayer alloc] initWithSession:session];
	[previewLayer setVideoGravity:AVLayerVideoGravityResizeAspect];

    // add the preview layer to the hierarchy
    CALayer *rootLayer = [previewView layer];
	[rootLayer setBackgroundColor:[[UIColor blackColor] CGColor]];
	[rootLayer addSublayer:previewLayer];

    captureController = [[RCCaptureManager alloc] init];

    isStarted = false;

    [[RCAVSessionManager sharedInstance] startSession];
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
    [frameRateSelector setHidden:true];
    [resolutionSelector setHidden:true];
}

- (void) captureDidStop
{
    [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
    [startStopButton setEnabled:true];
    [frameRateSelector setHidden:false];
    [resolutionSelector setHidden:false];
    AppDelegate * app = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    [app startFromHome];
}

- (IBAction)startStopClicked:(id)sender
{
    if (!isStarted)
    {
        int width = 640;
        int height = 480;
        int framerate = 30;
        if(frameRateSelector.selectedSegmentIndex == 1)
            framerate = 60;
        if(resolutionSelector.selectedSegmentIndex == 1) {
            width = 1280;
            height = 720;
        }
        else if(resolutionSelector.selectedSegmentIndex == 2) {
            width = 1920;
            height = 1080;
        }

        NSURL * fileurl = [AppDelegate timeStampedURLWithSuffix:[NSString stringWithFormat:@"_%d_%dHz.capture", width, framerate]];
        [startStopButton setTitle:@"Starting..." forState:UIControlStateNormal];
        [captureController startCaptureWithPath:fileurl.path withMaxFrameRate:framerate withWidth:width withHeight:height withDelegate:self];
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
