//
//  ViewController.m
//  RCCapture
//
//  Created by Brian on 9/27/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import "RC3DK.h"
#import "CaptureViewController.h"
#import "AppDelegate.h"
#import "CaptureFilename.h"
#import "RCCameraManager.h"

@interface CaptureViewController ()
{
    bool isStarted;
    AVCaptureVideoPreviewLayer * previewLayer;
    RCCaptureManager * captureController;
    NSURL * fileURL;
    int width;
    int height;
    int framerate;
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
    [[RCSensorManager sharedInstance] startAllSensors];
    [[RCAVSessionManager sharedInstance] startSession];
    [[RCVideoManager sharedInstance] startVideoCapture];

    isStarted = false;
    framerate = 30;
    width = 640;
    height = 480;
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

- (void) presentRenameAlert
{
    NSString * path = [fileURL.path stringByDeletingLastPathComponent];
    NSString * filename = [fileURL.path lastPathComponent];

    NSMutableDictionary * parameters = [NSMutableDictionary dictionaryWithDictionary:[CaptureFilename parseFilename:filename]];
    NSString * message = [NSString stringWithFormat:@"Add a length, path length, or rename\n%@x%@ @ %@Hz", parameters[@"width"], parameters[@"height"], parameters[@"framerate"]];

    UIAlertController * alert = [UIAlertController
                                 alertControllerWithTitle:@"Edit measurement"
                                 message:message
                                 preferredStyle:UIAlertControllerStyleAlert];

    UIAlertAction* save = [UIAlertAction actionWithTitle:@"Save" style:UIAlertActionStyleDefault
                                                 handler:^(UIAlertAction * action)
                           {
                               UITextField * nameTextField = alert.textFields[0];
                               UITextField * lengthTextField = alert.textFields[1];
                               UITextField * plTextField = alert.textFields[2];
                               if(nameTextField.hasText)
                                   parameters[@"basename"] = nameTextField.text;
                               if(lengthTextField.hasText)
                                   parameters[@"length"] = lengthTextField.text;
                               if(plTextField.hasText)
                                   parameters[@"pathlength"] = plTextField.text;
                               NSString * newFilename = [CaptureFilename filenameFromParameters:parameters];
                               NSString * newPath = [NSString pathWithComponents:[NSArray arrayWithObjects:path, newFilename, nil]];
                               [[NSFileManager defaultManager] moveItemAtPath:fileURL.path toPath:newPath error:nil];
                               [alert dismissViewControllerAnimated:YES completion:nil];
                           }];
    UIAlertAction* del = [UIAlertAction actionWithTitle:@"Delete" style:UIAlertActionStyleDestructive
                                                   handler:^(UIAlertAction * action)
                             {
                                 [[NSFileManager defaultManager] removeItemAtPath:fileURL.path error:nil];
                                 [alert dismissViewControllerAnimated:YES completion:nil];
                             }];

    [alert addTextFieldWithConfigurationHandler:^(UITextField *textField)
     {
         textField.placeholder = @"Filename";
         textField.text = parameters[@"basename"];
     }];

    [alert addTextFieldWithConfigurationHandler:^(UITextField *textField)
     {
         textField.placeholder = @"Length in cm";
         textField.keyboardType = UIKeyboardTypeDecimalPad;
     }];

    [alert addTextFieldWithConfigurationHandler:^(UITextField *textField)
     {
         textField.placeholder = @"Path length in cm";
         textField.keyboardType = UIKeyboardTypeDecimalPad;
     }];

    [alert addAction:save];
    [alert addAction:del];

    [self presentViewController:alert animated:YES completion:nil];
}

- (void) updateFramerateButton
{
    if(framerate == 30)
        frameRateSelector.selectedSegmentIndex = 0;
    else if(framerate == 60)
        frameRateSelector.selectedSegmentIndex = 1;
    else
        NSLog(@"Configured unknown framerate %d\n", framerate);
}
- (void) updateResolutionButton
{
    if(height == 480)
        resolutionSelector.selectedSegmentIndex = 0;
    else if(height == 720)
        resolutionSelector.selectedSegmentIndex = 1;
    else if(height == 1080)
        resolutionSelector.selectedSegmentIndex = 2;
    else
        NSLog(@"Unknown height %d", height);
}

- (IBAction)cameraConfigureClick:(id)sender
{
    if(frameRateSelector.selectedSegmentIndex == 0)
        framerate = 30;
    else if(frameRateSelector.selectedSegmentIndex == 1)
        framerate = 60;
    if(resolutionSelector.selectedSegmentIndex == 0) {
        width = 640;
        height = 480;
    }
    else if(resolutionSelector.selectedSegmentIndex == 1) {
        width = 1280;
        height = 720;
    }
    else if(resolutionSelector.selectedSegmentIndex == 2) {
        width = 1920;
        height = 1080;
    }
    [[RCAVSessionManager sharedInstance] configureCameraWithFrameRate:framerate withWidth:width withHeight:height];

    // Camera can refuse to be set to the framerate or size we specify
    AVCaptureDevice * device = [RCAVSessionManager sharedInstance].videoDevice;
    CMVideoDimensions sz = CMVideoFormatDescriptionGetDimensions(device.activeFormat.formatDescription);
    width = sz.width;
    height = sz.height;
    framerate = 1/CMTimeGetSeconds(device.activeVideoMaxFrameDuration);
    [self updateFramerateButton];
    [self updateResolutionButton];
}

- (void) focusFinished:(uint64_t)time withFocalLength:(float)focal_length
{
    [captureController startCaptureWithPath:fileURL.path];
    [startStopButton setTitle:@"Stop" forState:UIControlStateNormal];
    [frameRateSelector setHidden:true];
    [resolutionSelector setHidden:true];
}

- (IBAction)startStopClicked:(id)sender
{
    if (!isStarted)
    {
        fileURL = [AppDelegate timeStampedURLWithSuffix:[NSString stringWithFormat:@"_%d_%d_%dHz.capture", width, height, framerate]];
        [startStopButton setTitle:@"Starting..." forState:UIControlStateNormal];

        [[RCVideoManager sharedInstance] addDelegate:captureController];
        [[RCMotionManager sharedInstance] setDataDelegate:captureController];
        [[RCCameraManager sharedInstance] setVideoDevice:[[RCSensorManager sharedInstance] getVideoDevice]];

        __weak typeof(self) weakself = self;
        std::function<void (uint64_t, float)> callback = [weakself](uint64_t timestamp, float position)
        {
            [weakself focusFinished:timestamp withFocalLength:position];
        };
        [[RCCameraManager sharedInstance] focusOnceAndLockWithCallback:callback];
    }
    else
    {
        [startStopButton setEnabled:false];
        [startStopButton setTitle:@"Stopping..." forState:UIControlStateNormal];
        [captureController stopCapture];
        [self presentRenameAlert];
        [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
        [startStopButton setEnabled:true];
        [frameRateSelector setHidden:false];
        [resolutionSelector setHidden:false];
    }
    isStarted = !isStarted;
}

@end
