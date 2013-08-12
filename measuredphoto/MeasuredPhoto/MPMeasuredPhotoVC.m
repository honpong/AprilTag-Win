//
//  TMViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "MPMeasuredPhotoVC.h"
#import "math.h"

@implementation MPMeasuredPhotoVC
{
    BOOL useLocation;
    double lastTransitionTime;
    double lastFailTime;
    int filterStatusCode;
    BOOL isAligned;
    BOOL isMeasuring;
    
    MBProgressHUD *progressView;
    RCFeaturePoint* lastPointTapped;
    RCSensorFusionData* sfData;
}
@synthesize toolbar, thumbnail, shutterButton;

static const double stateTimeout = 2.;
static const double failTimeout = 2.;

typedef enum
{
    BUTTON_SHUTTER, BUTTON_DELETE
} ButtonImage;

enum state { ST_STARTUP, ST_READY, ST_FINISHED, ST_FINISHEDPAUSE, ST_ANY } currentState;
enum event { EV_RESUME, EV_FIRSTTIME, EV_CONVERGED, EV_STEADY_TIMEOUT, EV_VISIONFAIL, EV_FASTFAIL, EV_FAIL, EV_FAIL_EXPIRED, EV_TAP, EV_PAUSE, EV_CANCEL };

typedef struct { enum state state; enum event event; enum state newstate; } transition;

typedef struct
{
    enum state state;
    ButtonImage buttonImage;
    bool autofocus;
    bool videocapture;
    bool showMeasurements;
    bool avSession;
    bool isMeasuring;
    bool showTape;
    bool showDistance;
    bool features;
    bool progress;
    const char *title;
    const char *message;
    bool autohide;
} statesetup;

static statesetup setups[] =
{
    //                                    focus   vidcap  show-msmnts session measuring  shwdstc shwtape ftrs    prgrs
    { ST_STARTUP,       BUTTON_SHUTTER,   true,   false,  false,      true,   false,     false,  false,  false,  false,  "Startup",      "Slowly move left and right.", false},
    { ST_READY,         BUTTON_SHUTTER,   true,   true,   false,      true,   true,      true,   false,  true,   false,  "Ready",        "Slowly move left and right.", false },
    { ST_FINISHED,      BUTTON_DELETE,    false,  false,  true,       false,  false,     true,   true,   true,   false,  "Finished",     "", false },
    { ST_FINISHEDPAUSE, BUTTON_DELETE,    false,  false,  true,       false,  false,     false,  true,   true,   false,  "Finished",     "", false }
};

static transition transitions[] =
{
    { ST_STARTUP, EV_RESUME, ST_READY },
    { ST_READY, EV_TAP, ST_FINISHED },
    { ST_FINISHED, EV_PAUSE, ST_FINISHEDPAUSE },
    { ST_FINISHED, EV_TAP, ST_READY },
    { ST_ANY, EV_PAUSE, ST_STARTUP },
    { ST_ANY, EV_CANCEL, ST_STARTUP }
};

#define TRANS_COUNT (sizeof(transitions) / sizeof(*transitions))

- (void) validateStateMachine
{
    for(int i = 0; i < ST_ANY; ++i) {
        assert(setups[i].state == i);
    }
}

- (void) transitionToState:(int)newState
{
    if(currentState == newState) return;
    statesetup oldSetup = setups[currentState];
    statesetup newSetup = setups[newState];

    DLog(@"Transition from %s to %s", oldSetup.title, newSetup.title);

    if(oldSetup.autofocus && !newSetup.autofocus)
        [SESSION_MANAGER lockFocus];
    if(!oldSetup.autofocus && newSetup.autofocus)
        [SESSION_MANAGER unlockFocus];
    if(!oldSetup.avSession && newSetup.avSession)
        [SESSION_MANAGER startSession];
    if(oldSetup.avSession && !newSetup.avSession)
        [self endAVSessionInBackground];
    if(!oldSetup.videocapture && newSetup.videocapture)
        [self startVideoCapture];
    if(oldSetup.videocapture && !newSetup.videocapture)
        [self stopVideoCapture];
    if(oldSetup.features && !newSetup.features)
        [self.arView hideFeatures];
    if(!oldSetup.features && newSetup.features)
        [self.arView showFeatures];
    if(oldSetup.progress && !newSetup.progress)
        [self hideProgress];
    if(oldSetup.showMeasurements && !newSetup.showMeasurements)
        [self.arView.measurementsView clearMeasurements];
    if(!oldSetup.progress && newSetup.progress)
        [self showProgressWithTitle:[NSString stringWithCString:newSetup.title encoding:NSASCIIStringEncoding]];
    if(oldSetup.isMeasuring && !newSetup.isMeasuring)
        isMeasuring = NO;
    if(!oldSetup.isMeasuring && newSetup.isMeasuring)
        isMeasuring = YES;
    currentState = newState;

    NSString *message = [NSString stringWithFormat:[NSString stringWithCString:newSetup.message encoding:NSASCIIStringEncoding], filterStatusCode];
    [self showMessage:message withTitle:[NSString stringWithCString:newSetup.title encoding:NSASCIIStringEncoding] autoHide:newSetup.autohide];
    [self switchButtonImage:newSetup.buttonImage];
    
    lastTransitionTime = CACurrentMediaTime();
}

- (void)handleStateEvent:(int)event
{
    int newState = currentState;
    for(int i = 0; i < TRANS_COUNT; ++i) {
        if(transitions[i].state == currentState || transitions[i].state == ST_ANY) {
            if(transitions[i].event == event) {
                newState = transitions[i].newstate;
                DLog(@"State transition from %d to %d", currentState, newState);
                break;
            }
        }
    }
    if(newState != currentState) [self transitionToState:newState];
}

- (void)viewDidLoad
{
    LOGME
	[super viewDidLoad];
    
    isMeasuring = NO;
    
    [self validateStateMachine];
    
    useLocation = [LOCATION_MANAGER isLocationAuthorized] && [[NSUserDefaults standardUserDefaults] boolForKey:PREF_ADD_LOCATION];
    
    //setup screen tap detection
    UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapGesture:)];
    tapGesture.numberOfTapsRequired = 1;
    [self.arView addGestureRecognizer:tapGesture];
    
    self.messageBackground.layer.cornerRadius = 10.;
    
    SENSOR_FUSION.delegate = self;
    [VIDEO_MANAGER setupWithSession:SESSION_MANAGER.session];
    [SESSION_MANAGER startSession];
}

- (void) viewDidLayoutSubviews
{
    [self.arView initialize];
}

- (void)viewDidUnload
{
	LOGME
    [self setArView:nil];
    [self setShutterButton:nil];
    [self setThumbnail:nil];
	[super viewDidUnload];
}

- (void)viewWillAppear:(BOOL)animated
{
    [self.navigationController setToolbarHidden:YES animated:animated];
    [super viewWillAppear:animated];
}

- (void) viewDidAppear:(BOOL)animated
{
    LOGME
    [MPAnalytics logEvent:@"View.NewMeasurement"];
    [super viewDidAppear:animated];
    
    //register to receive notifications of pause/resume events
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleOrientationChange)
                                                 name:UIDeviceOrientationDidChangeNotification
                                               object:nil];
    [self handleResume];
}

- (void) viewWillDisappear:(BOOL)animated
{
    LOGME
    [super viewWillDisappear:animated];
    [self handleStateEvent:EV_CANCEL];
    [self endAVSessionInBackground];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) viewDidDisappear:(BOOL)animated
{
    [super viewDidDisappear:animated];
}

- (BOOL) shouldAutorotate
{
    return NO;
}

- (void) handleOrientationChange
{
    NSArray *toolbarH, *toolbarV, *thumbnailH, *thumbnailV, *shutterH, *shutterV;
    
    switch ([[UIDevice currentDevice] orientation])
    {
        case UIDeviceOrientationPortrait:
        {
            [self rotateUIByRadians:0];
            toolbarH = [NSLayoutConstraint constraintsWithVisualFormat:@"[toolbar(100)]-0-|" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            toolbarV = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|-0-[toolbar]-0-|" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            thumbnailH = [NSLayoutConstraint constraintsWithVisualFormat:@"[thumbnail(50)]-25-|" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(thumbnail)];
            thumbnailV = [NSLayoutConstraint constraintsWithVisualFormat:@"V:[thumbnail(50)]-25-|" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(thumbnail)];
            break;
        }
        case UIDeviceOrientationPortraitUpsideDown:
        {
            [self rotateUIByRadians:M_PI];
            toolbarH = [NSLayoutConstraint constraintsWithVisualFormat:@"|-0-[toolbar(100)]" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            toolbarV = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|-0-[toolbar]-0-|" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            thumbnailH = [NSLayoutConstraint constraintsWithVisualFormat:@"[thumbnail(50)]-25-|" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(thumbnail)];
            thumbnailV = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|-25-[thumbnail(50)]" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(thumbnail)];
            break;
        }
        case UIDeviceOrientationLandscapeLeft:
        {
            [self rotateUIByRadians:M_PI_2];
            toolbarH = [NSLayoutConstraint constraintsWithVisualFormat:@"|-0-[toolbar]-0-|" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            toolbarV = [NSLayoutConstraint constraintsWithVisualFormat:@"V:[toolbar(100)]-0-|" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            thumbnailH = [NSLayoutConstraint constraintsWithVisualFormat:@"|-25-[thumbnail(50)]" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(thumbnail)];
            thumbnailV = [NSLayoutConstraint constraintsWithVisualFormat:@"V:[thumbnail(50)]-25-|" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(thumbnail)];
            break;
        }
        case UIDeviceOrientationLandscapeRight:
        {
            [self rotateUIByRadians:-M_PI_2];
            toolbarH = [NSLayoutConstraint constraintsWithVisualFormat:@"|-0-[toolbar]-0-|" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            toolbarV = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|-0-[toolbar(100)]" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            thumbnailH = [NSLayoutConstraint constraintsWithVisualFormat:@"[thumbnail(50)]-25-|" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(thumbnail)];
            thumbnailV = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|-25-[thumbnail(50)]" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(thumbnail)];
            break;
        }
        default:
            break;
    }
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad && toolbarH && toolbarV)
    {
        [self.arView removeConstraints:self.arView.constraints];
        [self.arView addConstraints:toolbarH];
        [self.arView addConstraints:toolbarV];
        
        [toolbar removeConstraints:self.toolbar.constraints];
        [toolbar addConstraints:thumbnailH];
        [toolbar addConstraints:thumbnailV];
        
        shutterH = [NSLayoutConstraint constraintsWithVisualFormat:@"H:[toolbar]-(<=1)-[shutterButton]" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(shutterButton, toolbar)];
        shutterV = [NSLayoutConstraint constraintsWithVisualFormat:@"V:[toolbar]-(<=1)-[shutterButton]" options:NSLayoutFormatAlignAllCenterX metrics:nil views:NSDictionaryOfVariableBindings(shutterButton, toolbar)];
        [toolbar addConstraints:shutterH];
        [toolbar addConstraints:shutterV];
    }
}

- (void) rotateUIByRadians:(float)radians
{
    NSMutableArray* views = [NSMutableArray arrayWithObject:self.messageBackground];
    for (UIView* view in views)
    {
        view.transform = radians ? CGAffineTransformMakeRotation(radians) : CGAffineTransformIdentity;
    }
}

- (void)handlePause
{
	LOGME
    [self handleStateEvent:EV_PAUSE];
}

- (void)handleResume
{
	LOGME
    
//    if([RCCalibration hasCalibrationData]) {
    [self handleStateEvent:EV_RESUME];
//    } else {
//        [self handleStateEvent:EV_FIRSTTIME];
//    }
}

- (IBAction)handleShutterButton:(id)sender
{
    [self handleStateEvent:EV_TAP];
}

- (IBAction)handleThumbnail:(id)sender {
}

- (void) handleTapGesture:(UIGestureRecognizer *) sender
{
    if (sender.state != UIGestureRecognizerStateEnded) return;
    
    if (currentState == ST_FINISHED)
    {
        [self handleFeatureTapped:[sender locationInView:self.arView]];
    }    
}

- (void) handleFeatureTapped:(CGPoint)coordinateTapped
{
    RCFeaturePoint* pointTapped = [self.arView selectFeatureNearest:coordinateTapped];
    if(pointTapped)
    {
        if (lastPointTapped)
        {
            [self.arView.measurementsView drawMeasurementBetweenPointA:pointTapped andPointB:lastPointTapped];
            lastPointTapped = nil;
            [self.arView clearSelectedFeatures];
        }
        else
        {
            lastPointTapped = pointTapped;
        }
    }
}

- (void) startVideoCapture
{
    LOGME
    
    [SENSOR_FUSION startProcessingVideo];
    [VIDEO_MANAGER startVideoCapture];
    [VIDEO_MANAGER setDelegate:nil];
}

- (void) sensorFusionError:(RCSensorFusionError *)error
{
    double currentTime = CACurrentMediaTime();
    if(error.speed) {
        [self handleStateEvent:EV_FASTFAIL];
        [SENSOR_FUSION resetSensorFusion];
        [SENSOR_FUSION startProcessingVideo];
    } else if(error.other) {
        [self handleStateEvent:EV_FAIL];
        [SENSOR_FUSION resetSensorFusion];
        [SENSOR_FUSION startProcessingVideo];
    } else if(error.vision) {
        [self handleStateEvent:EV_VISIONFAIL];
    }
    if(lastFailTime == currentTime) {
        //in case we aren't changing states, update the error message
        NSString *message = [NSString stringWithFormat:[NSString stringWithCString:setups[currentState].message encoding:NSASCIIStringEncoding], filterStatusCode];
        [self showMessage:message withTitle:[NSString stringWithCString:setups[currentState].title encoding:NSASCIIStringEncoding] autoHide:setups[currentState].autohide];
    }
}

- (void) sensorFusionDidUpdate:(RCSensorFusionData*)data
{
    if (!isMeasuring) return;

    sfData = data;
    
    double currentTime = CACurrentMediaTime();
    double time_in_state = currentTime - lastTransitionTime;
    [self updateProgress:data.status.calibrationProgress];
    if(data.status.calibrationProgress >= 1.) {
        [self handleStateEvent:EV_CONVERGED];
    }
    if(data.status.isSteady && time_in_state > stateTimeout) [self handleStateEvent:EV_STEADY_TIMEOUT];
    
    double time_since_fail = currentTime - lastFailTime;
    if(time_since_fail > failTimeout) [self handleStateEvent:EV_FAIL_EXPIRED];

    if(data.sampleBuffer)
    {
        CVImageBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(data.sampleBuffer);
        if([self.arView.videoView beginFrame])
        {
            [self.arView.videoView displayPixelBuffer:pixelBuffer];
            [self.arView.videoView endFrame];
        }
        [self.arView.featuresLayer updateFeatures:data.featurePoints];
    }
}

- (void)stopVideoCapture
{
    LOGME
    [MPAnalytics logEvent:@"SensorFusion.Stop"];
    [VIDEO_MANAGER setDelegate:self.arView.videoView];
    [VIDEO_MANAGER stopVideoCapture];
    [self endAVSessionInBackground];
    if([SENSOR_FUSION isSensorFusionRunning])
        [SENSOR_FUSION stopProcessingVideo];
}

//- (void)postCalibrationToServer
//{
//    LOGME
//        
//    [SERVER_OPS
//     postDeviceCalibration:^{
//         DLog(@"postCalibrationToServer success");
//     }
//     onFailure:^(int statusCode) {
//         DLog(@"postCalibrationToServer failed with status code %i", statusCode);
//     }
//     ];
//}

- (void)showProgressWithTitle:(NSString*)title
{
    progressView = [[MBProgressHUD alloc] initWithView:self.view];
    progressView.mode = MBProgressHUDModeAnnularDeterminate;
    [self.view addSubview:progressView];
    progressView.labelText = title;
    [progressView show:YES];
}

- (void)hideProgress
{
    [progressView hide:YES];
}

- (void)updateProgress:(float)progress
{
    [progressView setProgress:progress];
}

//-(void)postMeasurement
//{
//    [newMeasurement
//     postToServer:
//     ^(int transId)
//     {
//         DLog(@"postMeasurement success callback");
//         newMeasurement.syncPending = NO;
//         [DATA_MANAGER saveContext];
//     }
//     onFailure:
//     ^(int statusCode)
//     {
//         //TODO: handle error
//         DLog(@"Post measurement failure callback");
//     }
//     ];
//}

- (void)showMessage:(NSString*)message withTitle:(NSString*)title autoHide:(BOOL)hide
{
    if (message && message.length > 0)
    {
        self.messageBackground.hidden = NO;
        self.messageLabel.hidden = NO;
        self.messageBackground.alpha = 1;
        self.messageLabel.alpha = 1;
        self.messageLabel.text = message ? message : @"";
        
        if (hide)
        {
            int const delayTime = 5;
            int const fadeTime = 2;
            
            [self fadeOut:self.messageBackground withDuration:fadeTime andWait:delayTime];
            [self fadeOut:self.messageLabel withDuration:fadeTime andWait:delayTime];
        }
    }
    else
    {
        [self hideMessage];
    }
}

- (void)hideMessage
{
    [self fadeOut:self.messageBackground withDuration:0.5 andWait:0];
    [self fadeOut:self.messageLabel withDuration:0.5 andWait:0];
}

-(void)fadeOut:(UIView*)viewToDissolve withDuration:(NSTimeInterval)duration andWait:(NSTimeInterval)wait
{
    [UIView beginAnimations: @"Fade Out" context:nil];
    
    // wait for time before begin
    [UIView setAnimationDelay:wait];
    
    // duration of animation
    [UIView setAnimationDuration:duration];
    viewToDissolve.alpha = 0.0;
    [UIView commitAnimations];
}

-(void)fadeIn:(UIView*)viewToFade withDuration:(NSTimeInterval)duration withAlpha:(float)alpha andWait:(NSTimeInterval)wait
{
    viewToFade.hidden = NO;
    viewToFade.alpha = 0;
    
    [UIView beginAnimations: @"Fade In" context:nil];
    
    // wait for time before begin
    [UIView setAnimationDelay:wait];
    
    // duration of animation
    [UIView setAnimationDuration:duration];
    viewToFade.alpha = alpha;
    [UIView commitAnimations];
}

-(void)fadeIn:(UIView*)viewToFade withDuration:(NSTimeInterval)duration andWait:(NSTimeInterval)wait
{
    [self fadeIn:viewToFade withDuration:duration withAlpha:1.0 andWait:wait];
}

- (void) prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
//    if ([[segue identifier] isEqualToString:@"toResult"])
//    {
//        TMResultsVC* resultsVC = [segue destinationViewController];
//        resultsVC.theMeasurement = newMeasurement;
//        resultsVC.prevView = self;
//    }
//    else if([[segue identifier] isEqualToString:@"toOptions"])
//    {
//        [self endAVSessionInBackground];
//        
//        TMOptionsVC *optionsVC = [segue destinationViewController];
//        optionsVC.theMeasurement = newMeasurement;
//        
//        [[segue destinationViewController] setDelegate:self];
//    }
}

- (void) endAVSessionInBackground
{
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        [SESSION_MANAGER endSession];
    });
}

- (void) switchButtonImage:(ButtonImage)imageType
{
    NSString* imageName;
    
    switch (imageType) {
        case BUTTON_DELETE:
            imageName = @"MobileMailSettings_trashmbox";
            break;
            
        default:
            imageName = @"PLCameraFloatingShutterButton";
            break;
    }
    
    UIImage* image = [UIImage imageNamed:imageName];
    CGRect buttonFrame = shutterButton.bounds;
    buttonFrame.size = image.size;
    shutterButton.frame = buttonFrame;
    [shutterButton setImage:[UIImage imageNamed:imageName] forState:UIControlStateNormal];
}

@end
