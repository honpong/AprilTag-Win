//
//  TMViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "MPCapturePhoto.h"
#import "math.h"
#import "MPYouTubeVideo.h"
#import "MPPhotoRequest.h"
#import <RCCore/RCCore.h>
#import "MPLoupe.h"

NSString * const MPUIOrientationDidChangeNotification = @"com.realitycap.MPUIOrientationDidChangeNotification";
static UIDeviceOrientation currentUIOrientation = UIDeviceOrientationPortrait;

@implementation MPCapturePhoto
{
    BOOL useLocation;
    double lastTransitionTime;
    double lastFailTime;
    int filterStatusCode;
    BOOL isAligned;
    BOOL isMeasuring;
    BOOL isQuestionDismissed;
    
    MBProgressHUD *progressView;
    RCSensorFusionData* lastSensorFusionDataWithImage;
    AFHTTPClient* httpClient;
    NSTimer* questionTimer;
    NSMutableArray *goodPoints;
}
@synthesize toolbar, thumbnail, shutterButton, messageLabel, questionLabel, questionSegButton, questionView, arView, containerView, instructionsView;

typedef NS_ENUM(int, AlertTag) {
    AlertTagTutorial = 0,
    AlertTagInstructions = 1
};

#pragma mark State Machine

static const double stateTimeout = 2.;
static const double failTimeout = 2.;

typedef enum
{
    BUTTON_SHUTTER, BUTTON_SHUTTER_DISABLED, BUTTON_DELETE, BUTTON_CANCEL
} ButtonImage;

enum state { ST_STARTUP, ST_READY, ST_MOVING, ST_ERROR, ST_FINISHED, ST_ANY } currentState;
enum event { EV_RESUME, EV_FIRSTTIME, EV_CONVERGED, EV_STEADY_TIMEOUT, EV_VISIONFAIL, EV_FASTFAIL, EV_FAIL, EV_FAIL_EXPIRED, EV_SHUTTER_TAP, EV_PAUSE, EV_CANCEL, EV_MOVE_DONE };

typedef struct { enum state state; enum event event; enum state newstate; } transition;

typedef struct
{
    enum state state;
    ButtonImage buttonImage;
    bool autofocus; // TODO: currently unused since 3dk now controls focus, remove parameter?
    bool videocapture;
    bool showMeasurements;
    bool avSession;
    bool isMeasuring;
    bool showBadFeatures;
    bool showSlideInstructions;
    bool features;
    bool progress;
    bool autohide;
    bool showStillPhoto;
    const char *title;
    const char *message;
} statesetup;

// dot and circle enabled
//static statesetup setups[] =
//{
//    //                  button image               focus   vidcap  shw-msmnts  session measuring  badfeat  instrct ftrs    prgrs    autohide stillPhoto   title         message
//    { ST_STARTUP,       BUTTON_SHUTTER_DISABLED,   true,   false,  false,      false,  false,     false,   false,  false,  false,   false,   false,       "Startup",    "Loading" },
//    { ST_READY,         BUTTON_SHUTTER,            false,  true,   false,      true,   true,      true,    false,  true,   false,   true,    false,       "Ready",      "Point the camera at the scene you want to capture, then press the button" },
//    { ST_MOVING,        BUTTON_CANCEL,             false,  true,   false,      true,   true,      true,    true,   true,   false,   false,   false,       "Moving",     "Move up, down, or sideways until the dot reaches the edge of the circle" },
//    { ST_ERROR,         BUTTON_DELETE,             true,   false,  true,       false,  false,     false,   false,  false,  false,   false,   false,       "Error",      "Whoops, something went wrong. Try again." },
//    { ST_FINISHED,      BUTTON_DELETE,             true,   false,  true,       false,  false,     false,   false,  false,  false,   true,    true,        "Finished",   "Tap anywhere to start a measurement, then tap again to finish it" }
//};

// dot and circle disabled
static statesetup setups[] =
{
    //                  button image               focus   vidcap  shw-msmnts  session measuring  badfeat  instrct ftrs    prgrs    autohide stillPhoto   title         message
    { ST_STARTUP,       BUTTON_SHUTTER_DISABLED,   true,   false,  false,      false,  false,     false,   false,  false,  false,   false,   false,       "Startup",    "Loading" },
    { ST_READY,         BUTTON_SHUTTER,            false,  true,   false,      true,   true,      true,    false,  true,   false,   true,    false,       "Ready",      "Point the camera at the scene you want to capture, then press the button" },
    { ST_MOVING,        BUTTON_SHUTTER,            false,  true,   false,      true,   true,      true,    false,  true,   false,   false,   false,       "Moving",     "Move up, down, or sideways. Press the button to finish" },
    { ST_ERROR,         BUTTON_DELETE,             true,   false,  true,       false,  false,     false,   false,  false,  false,   false,   false,       "Error",      "Whoops, something went wrong. Try again." },
    { ST_FINISHED,      BUTTON_DELETE,             true,   false,  true,       false,  false,     false,   false,  false,  false,   true,    true,        "Finished",   "Tap anywhere to start a measurement, then tap again to finish it" }
};

// dot and circle enabled
//static transition transitions[] =
//{
//    { ST_STARTUP, EV_RESUME, ST_READY },
//    { ST_READY, EV_SHUTTER_TAP, ST_MOVING },
//    { ST_MOVING, EV_SHUTTER_TAP, ST_READY },
//    { ST_MOVING, EV_MOVE_DONE, ST_FINISHED },
//    { ST_MOVING, EV_FAIL, ST_ERROR },
//    { ST_MOVING, EV_FASTFAIL, ST_ERROR },
//    { ST_MOVING, EV_VISIONFAIL, ST_ERROR },
//    { ST_ERROR, EV_SHUTTER_TAP, ST_READY },
//    { ST_FINISHED, EV_SHUTTER_TAP, ST_READY },
//    { ST_FINISHED, EV_PAUSE, ST_FINISHED },
//    { ST_ANY, EV_PAUSE, ST_STARTUP },
//    { ST_ANY, EV_CANCEL, ST_STARTUP }
//};

// dot and circle disabled
static transition transitions[] =
{
    { ST_STARTUP, EV_RESUME, ST_READY },
    { ST_READY, EV_SHUTTER_TAP, ST_MOVING },
    { ST_MOVING, EV_SHUTTER_TAP, ST_FINISHED },
    { ST_MOVING, EV_FAIL, ST_ERROR },
    { ST_MOVING, EV_FASTFAIL, ST_ERROR },
    { ST_MOVING, EV_VISIONFAIL, ST_ERROR },
    { ST_ERROR, EV_SHUTTER_TAP, ST_READY },
    { ST_FINISHED, EV_SHUTTER_TAP, ST_READY },
    { ST_FINISHED, EV_PAUSE, ST_FINISHED },
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

    if(!oldSetup.avSession && newSetup.avSession)
        [SESSION_MANAGER startSession];
    if(oldSetup.avSession && !newSetup.avSession)
        [self endAVSessionInBackground];
    if(!oldSetup.videocapture && newSetup.videocapture)
        [self startVideoCapture];
    if(oldSetup.videocapture && !newSetup.videocapture)
        [self stopVideoCapture];
    if(oldSetup.features && !newSetup.features)
        [arView hideFeatures]; [arView resetSelectedFeatures];
    if(!oldSetup.features && newSetup.features)
        [self.arView showFeatures];
    if(oldSetup.progress && !newSetup.progress)
        [self hideProgress];
    if(oldSetup.showMeasurements && !newSetup.showMeasurements)
        [self.arView.measurementsView clearMeasurements];
    if(!oldSetup.progress && newSetup.progress) // TODO: obsolete?
        [self showProgressWithTitle:@(newSetup.title)];
    if(oldSetup.isMeasuring && !newSetup.isMeasuring)
        isMeasuring = NO;
    if(!oldSetup.isMeasuring && newSetup.isMeasuring)
        isMeasuring = YES;
    if(oldSetup.showBadFeatures && !newSetup.showBadFeatures)
        self.arView.initializingFeaturesLayer.hidden = YES;
    if(!oldSetup.showBadFeatures && newSetup.showBadFeatures)
        self.arView.initializingFeaturesLayer.hidden = NO;
    if(!oldSetup.showSlideInstructions && newSetup.showSlideInstructions)
        instructionsView.hidden = NO;
    if(oldSetup.showSlideInstructions && !newSetup.showSlideInstructions)
        instructionsView.hidden = YES;
    if(!oldSetup.showStillPhoto && newSetup.showStillPhoto)
    {
        arView.photoView.hidden = NO;
        arView.magGlassEnabled = YES;
    }
    if(oldSetup.showStillPhoto && !newSetup.showStillPhoto)
    {
        arView.photoView.hidden = YES;
        arView.magGlassEnabled = NO;
    }
    if(currentState == ST_READY && newState == ST_MOVING)
        [self handleMoveStart];
    if(currentState == ST_MOVING && newState == ST_FINISHED)
        [self handleMoveFinished];
    if(currentState == ST_FINISHED && newState == ST_READY)
        [self handlePhotoDeleted];
    
    NSString* message = @(newSetup.message);
    [self showMessage:message withTitle:@"" autoHide:newSetup.autohide];
    
    [self switchButtonImage:newSetup.buttonImage];
    
    lastTransitionTime = CACurrentMediaTime();
    currentState = newState;
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

#pragma mark View Controller

- (void)viewDidLoad
{
    LOGME
	[super viewDidLoad];
    
    // determine if we have an internet connection for playing the tutorial video
    if ([[NSUserDefaults standardUserDefaults] integerForKey:PREF_TUTORIAL_ANSWER] == MPTutorialAnswerNotNow)
    {
        httpClient = [AFHTTPClient clientWithBaseURL:[NSURL URLWithString:@"http://www.youtube.com"]];
        __weak MPCapturePhoto* weakSelf = self;
        [httpClient setReachabilityStatusChangeBlock:^(AFNetworkReachabilityStatus status) {
            if (status == AFNetworkReachabilityStatusReachableViaWiFi || status == AFNetworkReachabilityStatusReachableViaWWAN)
            {
                [weakSelf showTutorialDialog];
            }
            else if ([[NSUserDefaults standardUserDefaults] boolForKey:PREF_SHOW_INSTRUCTIONS])
            {
                [weakSelf showInstructionsDialog];
            }
            [[RCHTTPClient sharedInstance] setReachabilityStatusChangeBlock:nil];
        }];
    } else if ([[NSUserDefaults standardUserDefaults] boolForKey:PREF_SHOW_INSTRUCTIONS])
    {
        [self showInstructionsDialog];
    }
    
    isMeasuring = NO;
    
    arView.delegate = self;
    instructionsView.delegate = self;
    containerView.delegate = arView;
    
    [self validateStateMachine];
    
    useLocation = [LOCATION_MANAGER isLocationAuthorized] && [[NSUserDefaults standardUserDefaults] boolForKey:PREF_ADD_LOCATION];
    
    [VIDEO_MANAGER setupWithSession:SESSION_MANAGER.session];
    [VIDEO_MANAGER setDelegate:self.arView.videoView];
    [SESSION_MANAGER startSession];
    
    if (SYSTEM_VERSION_LESS_THAN(@"7")) questionSegButton.tintColor = [UIColor darkGrayColor];
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
    self.screenName = @"TakeMeasuredPhoto"; // must go before call to super
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
    
    [questionView hideInstantly];
    [self handleResume];
}

- (void) viewWillDisappear:(BOOL)animated
{
    LOGME
    [super viewWillDisappear:animated];
    [self handleStateEvent:EV_CANCEL];
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

+ (UIDeviceOrientation) getCurrentUIOrientation
{
    return currentUIOrientation;
}

- (void) handleOrientationChange
{
    UIDeviceOrientation newOrientation = [[UIDevice currentDevice] orientation];
    
    if (newOrientation == UIDeviceOrientationPortrait || newOrientation == UIDeviceOrientationPortraitUpsideDown || newOrientation == UIDeviceOrientationLandscapeLeft || newOrientation == UIDeviceOrientationLandscapeRight)
    {
        currentUIOrientation = newOrientation;
        [self.view rotateChildViews:currentUIOrientation];
        [[NSNotificationCenter defaultCenter] postNotificationName:MPUIOrientationDidChangeNotification object:self];
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
    SENSOR_FUSION.delegate = self;
    [self handleStateEvent:EV_RESUME];
    [self handleOrientationChange]; // ensures that UI is in correct orientation
}

- (IBAction)handleShutterButton:(id)sender
{
    [self handleStateEvent:EV_SHUTTER_TAP];
}

- (IBAction)handleThumbnail:(id)sender {
}

- (IBAction)handleQuestionButton:(id)sender
{
    questionLabel.text = @"Thanks!";
    [questionView
     hideWithDelay:.5
     onCompletion:^(BOOL finished){
         questionLabel.text = @"Are the measurements accurate?";
         [questionSegButton setSelectedSegmentIndex:-1]; // clear selection
     }];
    
    UISegmentedControl* button = (UISegmentedControl*)sender;
    switch (button.selectedSegmentIndex)
    {
        case 0:
            // Pretty close
            [self postAnswer:YES];
            break;
        case 1:
            // Not really
            [self postAnswer:NO];
            break;
        case 2:
            // Don't show again
            [[NSUserDefaults standardUserDefaults] setBool:NO forKey:PREF_SHOW_ACCURACY_QUESTION];
            [[NSUserDefaults standardUserDefaults] synchronize];
            break;
        default:
            break;
    }
    
    isQuestionDismissed = YES;
}

- (IBAction)handleQuestionCloseButton:(id)sender
{
    [questionView hideInstantly];
    isQuestionDismissed = YES;
}

- (void) handleTapGesture:(UIGestureRecognizer *) sender
{
    if (sender.state != UIGestureRecognizerStateEnded) return;
    
    CGPoint tappedPoint = [sender locationInView:self.arView];
    if (currentState == ST_FINISHED)
    {
        [arView handleFeatureTapped:tappedPoint];
    }
    else if (currentState == ST_READY)
    {
//        CGPoint point = [self.arView.featuresLayer cameraPointFromScreenPoint:tappedPoint];
//        [SENSOR_FUSION selectUserFeatureWithX:point.x withY:point.y];
    }
}

- (void) handleMoveStart
{
    LOGME
    [SENSOR_FUSION resetOrigin];
    [SENSOR_FUSION startProcessingStereo];
}

- (void) handleMoveFinished
{
    LOGME
    isQuestionDismissed = NO;
    [instructionsView moveDotToCenter];
    [arView.photoView setImageWithSampleBuffer:lastSensorFusionDataWithImage.sampleBuffer];
}

- (void) handlePhotoDeleted
{
    [questionView hideWithDelay:0 onCompletion:nil];
    [self hideMessage];
    
    // TODO for testing only
//    TMMeasuredPhoto* mp = [[TMMeasuredPhoto alloc] init];
//    mp.appVersion = @"1.2";
//    mp.appBuildNumber = @5;
//    mp.featurePoints = [MPPhotoRequest transcribeFeaturePoints:goodPoints];
//    mp.imageData = [MPPhotoRequest sampleBufferToNSData:lastSensorFusionDataWithImage.sampleBuffer];
//    [[MPPhotoRequest lastRequest] sendMeasuredPhoto:mp];
}

- (void) featureTapped
{
    if (questionTimer && questionTimer.isValid) [questionTimer invalidate];
}

- (void) measurementCompleted
{
    if ([[NSUserDefaults standardUserDefaults] boolForKey:PREF_SHOW_ACCURACY_QUESTION] && !isQuestionDismissed)
    {
        questionTimer = [NSTimer
                         scheduledTimerWithTimeInterval:2.
                         target:questionView
                         selector:@selector(showAnimated)
                         userInfo:nil
                         repeats:false];
    }
}

- (void) showTutorialDialog
{
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Tutorial Video"
                                                    message:@"Watch this short video to learn how to use the app."
                                                   delegate:self
                                          cancelButtonTitle:@"Don't ask again"
                                          otherButtonTitles:@"OK (recommended)", @"Not now", nil];
    alert.tag = AlertTagTutorial;
    [alert show];
}

// TODO: obsolete
- (void) showInstructionsDialog
{
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Instructions"
                                                    message:@"Hold the device firmly with two hands. Keep the camera pointed at what you want to measure and slide the device left, right, up and down. When some of the dots turn blue, then press the shutter button to take the photo."
                                                   delegate:self
                                          cancelButtonTitle:@"Don't show again"
                                          otherButtonTitles:@"OK", nil];
    alert.tag = AlertTagInstructions;
    [alert show];
}

- (void) alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if (alertView.tag == AlertTagTutorial)
    {
        if (buttonIndex == 0) // don't ask again
        {
            [[NSUserDefaults standardUserDefaults] setInteger:MPTutorialAnswerDontAskAgain forKey:PREF_TUTORIAL_ANSWER];
            if ([[NSUserDefaults standardUserDefaults] boolForKey:PREF_SHOW_INSTRUCTIONS]) [self showInstructionsDialog];
        }
        else if (buttonIndex == 1) // YES
        {
            [[NSUserDefaults standardUserDefaults] setInteger:MPTutorialAnswerYes forKey:PREF_TUTORIAL_ANSWER];
            [[NSUserDefaults standardUserDefaults] setBool:NO forKey:PREF_SHOW_INSTRUCTIONS];
            MPYouTubeVideo* vc = [self.storyboard instantiateViewControllerWithIdentifier:@"YouTubeVideo"];
            self.view.window.rootViewController = vc;
        }
        else if (buttonIndex == 2) // not now
        {
            [[NSUserDefaults standardUserDefaults] setInteger:MPTutorialAnswerNotNow forKey:PREF_TUTORIAL_ANSWER];
            if ([[NSUserDefaults standardUserDefaults] boolForKey:PREF_SHOW_INSTRUCTIONS]) [self showInstructionsDialog];
        }
    }
    else if (alertView.tag == AlertTagInstructions)
    {
        if (buttonIndex == 0) // don't show again
        {
            [[NSUserDefaults standardUserDefaults] setBool:NO forKey:PREF_SHOW_INSTRUCTIONS];
        }
    }
}

#pragma mark 3DK Stuff

- (void) startVideoCapture
{
    LOGME
    
    [SENSOR_FUSION startProcessingVideoWithDevice:[SESSION_MANAGER videoDevice]];
    [VIDEO_MANAGER startVideoCapture];
    [VIDEO_MANAGER setDelegate:nil];
}

- (void)stopVideoCapture
{
    LOGME
    [VIDEO_MANAGER setDelegate:self.arView.videoView];
    [VIDEO_MANAGER stopVideoCapture];
    if([SENSOR_FUSION isSensorFusionRunning]) {
        [SENSOR_FUSION stopProcessingStereo];
        [SENSOR_FUSION stopProcessingVideo];
    }
}

#pragma mark RCSensorFusionDelegate

- (void) sensorFusionError:(NSError *)error
{
    DLog(@"ERROR code %i %@", error.code, error.debugDescription);
    double currentTime = CACurrentMediaTime();
    if(error.code == RCSensorFusionErrorCodeTooFast) {
        [self handleStateEvent:EV_FASTFAIL];
    } else if(error.code == RCSensorFusionErrorCodeOther) {
        [self handleStateEvent:EV_FAIL];
    } else if(error.code == RCSensorFusionErrorCodeVision) {
        [self handleStateEvent:EV_VISIONFAIL];
    }
    if(lastFailTime == currentTime) {
        //in case we aren't changing states, update the error message
        NSString *message = [NSString stringWithFormat:@(setups[currentState].message), filterStatusCode];
        [self showMessage:message withTitle:@(setups[currentState].title) autoHide:setups[currentState].autohide];
    }
}

- (void) sensorFusionDidUpdate:(RCSensorFusionData*)data
{
    if (!isMeasuring) return;

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
        lastSensorFusionDataWithImage = data;
        
        [self.arView.videoView displaySampleBuffer:data.sampleBuffer];
        
        goodPoints = [[NSMutableArray alloc] init];
        NSMutableArray *badPoints = [[NSMutableArray alloc] init];
        
        for(RCFeaturePoint *feature in data.featurePoints)
        {
            if((feature.originalDepth.standardDeviation / feature.originalDepth.scalar < .01 || feature.originalDepth.standardDeviation < .004) && feature.initialized)
                [goodPoints addObject:feature];
            else
                [badPoints addObject:feature];
        }
        
        [self.arView.featuresLayer updateFeatures:goodPoints];
        [self.arView.initializingFeaturesLayer updateFeatures:badPoints];
    }
    
    if (currentState == ST_MOVING) [instructionsView updateDotPosition:data.transformation];
}

/** delegate method of MPInstructionsViewDelegate. tells us when the dot has reached the edge of the circle. */
- (void) moveComplete
{
//    [self handleStateEvent:EV_MOVE_DONE];
}

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

- (void)showMessage:(NSString*)message withTitle:(NSString*)title autoHide:(BOOL)hide
{
    if (message && message.length > 0)
    {
        self.messageLabel.hidden = NO;
        self.messageLabel.alpha = 1;
        self.messageLabel.text = message ? message : @"";
        
        if (hide) [self fadeOut:self.messageLabel withDuration:2 andWait:5];
    }
    else
    {
        [self hideMessage];
    }
}

- (void)hideMessage
{
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
            shutterButton.alpha = 1.;
            shutterButton.enabled = YES;
            break;
            
        case BUTTON_SHUTTER_DISABLED:
            imageName = @"PLCameraFloatingShutterButton";
            shutterButton.alpha = .3;
            shutterButton.enabled = NO;
            break;
            
        case BUTTON_CANCEL:
            imageName = @"BackButton";
            shutterButton.alpha = 1.;
            shutterButton.enabled = YES;
            break;
            
        default:
            imageName = @"PLCameraFloatingShutterButton";
            shutterButton.alpha = 1.;
            shutterButton.enabled = YES;
            break;
    }
    
    UIImage* image = [UIImage imageNamed:imageName];
    CGRect buttonFrame = shutterButton.bounds;
    buttonFrame.size = image.size;
    shutterButton.frame = buttonFrame;
    [shutterButton setImage:[UIImage imageNamed:imageName] forState:UIControlStateNormal];
}

- (void) postAnswer:(BOOL)isAccurate
{
    LOGME;
    
    [MPAnalytics logEventWithCategory:kAnalyticsCategoryFeedback withAction:@"Accuracy" withLabel:nil withValue:isAccurate ? @1 : @0];
    [MPAnalytics dispatch];
    
    NSString* vendorId = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
    NSString* answer = isAccurate ? @"true" : @"false";
    NSString* jsonString = [NSString stringWithFormat:@"{ \"id\":\"%@\", \"is_accurate\": \"%@\" }", vendorId, answer];
    NSDictionary* postParams = @{ @"secret": @"BensTheDude", JSON_KEY_FLAG:@(JsonBlobFlagAccuracyQuestion), JSON_KEY_BLOB: jsonString };
    
    [HTTP_CLIENT
     postPath:API_DATUM_LOGGED
     parameters:postParams
     success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         DLog(@"POST Response\n%@", operation.responseString);
     }
     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         if (operation.response.statusCode)
         {
             DLog(@"Failed to POST. Status: %i %@", operation.response.statusCode, operation.responseString);
             NSString *requestBody = [[NSString alloc] initWithData:operation.request.HTTPBody encoding:NSUTF8StringEncoding];
             DLog(@"Failed request body:\n%@", requestBody);
         }
         else
         {
             DLog(@"Failed to POST.\n%@", error);
         }
     }
     ];
}

@end
