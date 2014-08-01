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
#import "MPLoupe.h"
#import "MPLocalMoviePlayer.h"
#import "UIImage+MPImageFile.h"
#import "MPSurveyAnswer.h"
#import "RCCore/RCStereo.h"
#import "RCCore/RCSensorDelegate.h"
@import MediaPlayer;
#import "MPDMeasuredPhoto.h"
#import "MPDLocation.h"
#import "CoreData+MagicalRecord.h"
#import "MBProgressHUD.h"
#import "MPDMeasuredPhoto+MPDMeasuredPhotoExt.h"
#import "MPEditPhoto.h"
#import "MPGalleryController.h"
#import "MPVideoPreview.h"

static UIDeviceOrientation currentUIOrientation = UIDeviceOrientationPortrait;

@implementation MPCapturePhoto
{
    BOOL useLocation;
    double lastTransitionTime;
    int filterStatusCode;
    BOOL isAligned;
    BOOL isQuestionDismissed;
    
    MBProgressHUD *progressView;
    RCSensorFusionData* lastSensorFusionDataWithImage;
    AFHTTPClient* httpClient;
    NSTimer* questionTimer;
    NSMutableArray *goodPoints;
    UIImage * lastImage;

    id<RCSensorDelegate> sensorDelegate;
    
    MPDMeasuredPhoto* measuredPhoto;
}
@synthesize toolbar, thumbnail, shutterButton, messageLabel, questionLabel, questionSegButton, questionView, arView, containerView, instructionsView;

typedef NS_ENUM(int, AlertTag) {
    AlertTagTutorial = 0,
    AlertTagInstructions = 1
};

#pragma mark - State Machine

typedef enum
{
    BUTTON_SHUTTER, BUTTON_SHUTTER_DISABLED, BUTTON_DELETE, BUTTON_CANCEL
} ButtonImage;

typedef NS_ENUM(int, SpinnerType) {
    SpinnerTypeNone,
    SpinnerTypeDeterminate,
    SpinnerTypeIndeterminate
};

enum state { ST_STARTUP, ST_READY, ST_INITIALIZING, ST_MOVING, ST_CAPTURE, ST_PROCESSING, ST_ERROR, ST_FINISHED, ST_ANY } currentState;
enum event { EV_RESUME, EV_FIRSTTIME, EV_VISIONFAIL, EV_FASTFAIL, EV_FAIL, EV_SHUTTER_TAP, EV_PAUSE, EV_CANCEL, EV_MOVE_DONE, EV_PROCESSING_FINISHED, EV_INITIALIZED, EV_STEREOFAIL };

typedef struct { enum state state; enum event event; enum state newstate; } transition;

typedef struct
{
    enum state state;
    ButtonImage buttonImage;
    bool sensorCapture;
    bool sensorFusion;
    bool showMeasurements;
    bool showBadFeatures;
    bool showSlideInstructions;
    bool features;
    SpinnerType progress;
    bool autohide; // auto hide message
    bool showStillPhoto;
    bool stereo;
    const char *title;
    const char *message;
} statesetup;

static statesetup setups[] =
{
    //                  button image               sensors fusion   shw-msmnts  badfeat  instrct ftrs    prgrs                     autohide stillPhoto  stereo  title           message
    { ST_STARTUP,       BUTTON_SHUTTER_DISABLED,   false,  false,   false,      false,   false,  false,  SpinnerTypeNone,          false,   false,      false,  "Startup",      "Loading" },
    { ST_READY,         BUTTON_SHUTTER,            true,   false,   false,      false,   false,  false,  SpinnerTypeNone,          true,    false,      false,  "Ready",        "Point the camera at the scene you want to capture, then press the button." },
    { ST_INITIALIZING,  BUTTON_SHUTTER_DISABLED,   true,   true,    false,      true,    false,  true,   SpinnerTypeDeterminate,   true,    false,      false,  "Initializing", "Hold still" },
    { ST_MOVING,        BUTTON_DELETE,             true,   true,    false,      true,    true,   true,   SpinnerTypeNone,          false,   false,      true,   "Moving",       "Move up, down, or sideways. Press the button to cancel." },
    { ST_CAPTURE,       BUTTON_SHUTTER,            true,   true,    false,      true,    true,   true,   SpinnerTypeNone,          false,   false,      true,   "Capture",      "Press the button to capture a photo." },
    { ST_PROCESSING,    BUTTON_DELETE,             false,  false,   false,      false,   false,  false,  SpinnerTypeDeterminate,   true,    true,       true,   "Processing",   "Please wait" },
    { ST_ERROR,         BUTTON_DELETE,             false,  false,   true,       false,   false,  false,  SpinnerTypeNone,          false,   false,      false,  "Error",        "Whoops, something went wrong. Try again." },
    { ST_FINISHED,      BUTTON_DELETE,             false,  false,   true,       false,   false,  false,  SpinnerTypeNone,          true,    true,       false,  "Finished",     "Tap anywhere to start a measurement, then tap again to finish it" }
};

static transition transitions[] =
{
    { ST_STARTUP, EV_RESUME, ST_READY },
    { ST_READY, EV_SHUTTER_TAP, ST_INITIALIZING },
    { ST_INITIALIZING, EV_INITIALIZED, ST_MOVING },
    { ST_MOVING, EV_SHUTTER_TAP, ST_READY },
    { ST_MOVING, EV_MOVE_DONE, ST_CAPTURE },
    { ST_MOVING, EV_FAIL, ST_ERROR },
    { ST_MOVING, EV_FASTFAIL, ST_ERROR },
    { ST_CAPTURE, EV_SHUTTER_TAP, ST_PROCESSING },
    { ST_PROCESSING, EV_PROCESSING_FINISHED, ST_FINISHED },
    { ST_PROCESSING, EV_STEREOFAIL, ST_ERROR },
    { ST_CAPTURE, EV_FAIL, ST_ERROR },
    { ST_CAPTURE, EV_FASTFAIL, ST_ERROR },
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

    DLog(@"Transitioning from %s to %s", oldSetup.title, newSetup.title);

    if(!oldSetup.sensorCapture && newSetup.sensorCapture)
        [self startSensors];
    if(!oldSetup.sensorFusion && newSetup.sensorFusion)
        [self startSensorFusion];
    if(oldSetup.sensorCapture && !newSetup.sensorCapture)
        [self stopSensors];
    if(oldSetup.sensorFusion && !newSetup.sensorFusion)
        [self stopSensorFusion];
    if(oldSetup.features && !newSetup.features)
        [arView hideFeatures]; [arView resetSelectedFeatures];
    if(!oldSetup.features && newSetup.features)
        [self.arView showFeatures];
    if(oldSetup.progress != newSetup.progress)
    {
        if (newSetup.progress == SpinnerTypeDeterminate)
            [self showProgressWithTitle:@(newSetup.message)];
        else if (newSetup.progress == SpinnerTypeIndeterminate)
            [self showIndeterminateProgress:@(newSetup.message)];
        else
            [self hideProgress];
    }
    if(oldSetup.showMeasurements && !newSetup.showMeasurements)
        [self.arView.measurementsView clearMeasurements];
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
    if(currentState == ST_MOVING && newState == ST_CAPTURE)
        [self handleMoveFinished];
    if(currentState == ST_CAPTURE && newState == ST_PROCESSING)
        [self handleCaptureFinished];
    if(currentState == ST_FINISHED && newState == ST_READY)
        [self handlePhotoDeleted];
    
    if (newSetup.progress == SpinnerTypeNone)
    {
        NSString* message = @(newSetup.message);
        [self showMessage:message withTitle:@"" autoHide:newSetup.autohide];
    }
    else
    {
        [self hideMessage];
    }
    
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
//                DLog(@"State transition from %d to %d", currentState, newState);
                break;
            }
        }
    }
    if(newState != currentState) [self transitionToState:newState];
}

#pragma mark - View Controller

- (void)viewDidLoad
{
    LOGME
	[super viewDidLoad];
    
    // determine if we have an internet connection for playing the tutorial video
//    if ([[NSUserDefaults standardUserDefaults] integerForKey:PREF_TUTORIAL_ANSWER] == MPTutorialAnswerNotNow)
//    {
//            MPLocalMoviePlayer* movieController = [self.storyboard instantiateViewControllerWithIdentifier:@"MoviePlayer"];
//            [self presentMoviePlayerViewControllerAnimated:movieController];
//    } else if ([[NSUserDefaults standardUserDefaults] boolForKey:PREF_SHOW_INSTRUCTIONS])
//    {
//        [self showInstructionsDialog];
//    }
    
    arView.delegate = self;
    instructionsView.delegate = self;
    containerView.delegate = arView;
    
    sensorDelegate = [SensorDelegate sharedInstance];
    
    [self validateStateMachine];
    
    useLocation = [LOCATION_MANAGER isLocationAuthorized] && [[NSUserDefaults standardUserDefaults] boolForKey:PREF_ADD_LOCATION];
    
    [[sensorDelegate getVideoProvider] setDelegate:self.arView.videoView];
    
    if (SYSTEM_VERSION_LESS_THAN(@"7")) questionSegButton.tintColor = [UIColor darkGrayColor];
    
    progressView = [[MBProgressHUD alloc] initWithView:self.view];
    progressView.mode = MBProgressHUDModeAnnularDeterminate;
    [self.containerView addSubview:progressView];
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
        
    if ([[NSUserDefaults.standardUserDefaults objectForKey:PREF_IS_FIRST_START]  isEqual: @YES])
    {
        [NSUserDefaults.standardUserDefaults setObject:@NO forKey:PREF_IS_FIRST_START];
        [self gotoTutorialVideo];
    }
    
    [[NSNotificationCenter defaultCenter] postNotificationName:MPCapturePhotoDidAppearNotification object:nil];
}

- (void) gotoTutorialVideo
{
    NSURL *movieURL = [[NSBundle mainBundle] URLForResource:@"TrueMeasureTutorial" withExtension:@"mp4"];
    MPLocalMoviePlayer* movieViewController = [[MPLocalMoviePlayer alloc] initWithContentURL:movieURL];
    movieViewController.modalPresentationStyle = UIModalPresentationFullScreen;
    [self presentMoviePlayerViewControllerAnimated:movieViewController];
    
    [[NSNotificationCenter defaultCenter] removeObserver:movieViewController  name:MPMoviePlayerPlaybackDidFinishNotification object:movieViewController.moviePlayer]; // needed to receive the notification below
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(moviePlayerWillExitFullScreen:) name:MPMoviePlayerPlaybackDidFinishNotification object:nil];
}

- (void) moviePlayerWillExitFullScreen:(NSNotification*)notification
{
    NSNumber* reason = (NSNumber*)[notification.userInfo objectForKey:MPMoviePlayerPlaybackDidFinishReasonUserInfoKey];
    if (reason.intValue == MPMovieFinishReasonUserExited)
    {
        [self dismissMoviePlayerViewControllerAnimated];
        [self handleResume];
    }
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

- (NSUInteger)supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskPortrait;
}

- (void) handleOrientationChange
{
    UIDeviceOrientation newOrientation = [[UIDevice currentDevice] orientation];
    if (currentUIOrientation != newOrientation && (newOrientation == UIDeviceOrientationPortrait || newOrientation == UIDeviceOrientationPortraitUpsideDown || newOrientation == UIDeviceOrientationLandscapeLeft || newOrientation == UIDeviceOrientationLandscapeRight))
    {
        currentUIOrientation = newOrientation;
        [self setOrientation:currentUIOrientation animated:YES];
    }
}

- (void) setOrientation:(UIDeviceOrientation)orientation animated:(BOOL)animated
{
    [self.view rotateChildViews:orientation animated:animated];
    NSValue *value = [NSValue value: &orientation withObjCType: @encode(enum UIDeviceOrientation)];
    [[NSNotificationCenter defaultCenter] postNotificationName:MPUIOrientationDidChangeNotification object:value];
}

- (void)handlePause
{
	LOGME
    [self handleStateEvent:EV_PAUSE];
}

- (void)handleResume
{
	LOGME
    [self handleStateEvent:EV_RESUME];
    [self handleOrientationChange]; // ensures that UI is in correct orientation
}

- (IBAction)handleShutterButton:(id)sender
{
    [self handleStateEvent:EV_SHUTTER_TAP];
}

- (IBAction)handleThumbnail:(id)sender
{
    [self gotoGallery];
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
            [MPSurveyAnswer postAnswer:YES];
            break;
        case 1:
            // Not really
            [MPSurveyAnswer postAnswer:NO];
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
}

- (void) handleMoveFinished
{
    LOGME
//    [instructionsView moveDotToCenter];
}

- (void) handleCaptureFinished
{
    LOGME
    
    measuredPhoto = [self saveMeasuredPhoto];

    RCStereo * stereo = [RCStereo sharedInstance];
    [stereo setGuid: measuredPhoto.id_guid];
    [stereo processFrame:lastSensorFusionDataWithImage withFinal:true];
    [stereo setOrientation:[MPCapturePhoto getCurrentUIOrientation]];
    stereo.delegate = self;
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
        [measuredPhoto writeImagetoJpeg:lastSensorFusionDataWithImage.sampleBuffer withOrientation:[MPCapturePhoto getCurrentUIOrientation]];
        [stereo preprocess];
    });
}

- (void) gotoEditPhotoScreen
{
    if ([self.presentingViewController isKindOfClass:[MPGalleryController class]])
    {
        MPGalleryController* galleryController = (MPGalleryController*)self.presentingViewController;
        MPEditPhoto* editPhotoController = galleryController.editPhotoController;
        editPhotoController.measuredPhoto = measuredPhoto;
        editPhotoController.transitioningDelegate = nil; // remove fade transition
        [self presentViewController:editPhotoController animated:YES completion:nil];
    }
    else if ([self.presentingViewController isKindOfClass:[MPEditPhoto class]])
    {
        MPEditPhoto* editPhotoController = (MPEditPhoto*)self.presentingViewController;
        editPhotoController.measuredPhoto = measuredPhoto;
        editPhotoController.transitioningDelegate = nil; // remove fade transition
        [self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
    }
}

- (void) gotoGallery
{
    [self.arView.videoView animateClosed:^(BOOL finished) {
        dispatch_async(dispatch_get_main_queue(), ^{
            if ([self.presentingViewController isKindOfClass:[MPGalleryController class]])
            {
                [self.presentingViewController dismissViewControllerAnimated:NO completion:nil];
            }
            else if ([self.presentingViewController isKindOfClass:[MPEditPhoto class]])
            {
                [self.presentingViewController.presentingViewController dismissViewControllerAnimated:NO completion:nil];
            }
        });
    }];
}

- (void) handlePhotoDeleted
{
    [questionView hideWithDelay:0 onCompletion:nil];
    [self hideMessage];
    [STEREO reset];
    
    // TODO for testing only
//    TMMeasuredPhoto* mp = [[TMMeasuredPhoto alloc] init];
//    mp.appVersion = @"1.2";
//    mp.appBuildNumber = @5;
//    mp.featurePoints = [MPPhotoRequest transcribeFeaturePoints:goodPoints];
//    mp.imageData = [MPPhotoRequest sampleBufferToNSData:lastSensorFusionDataWithImage.sampleBuffer];
//    [[MPPhotoRequest lastRequest] sendMeasuredPhoto:mp];
}

- (MPDMeasuredPhoto*) saveMeasuredPhoto
{
    MPDMeasuredPhoto* cdMeasuredPhoto = [MPDMeasuredPhoto MR_createEntity];
    
    [CONTEXT MR_saveToPersistentStoreWithCompletion:^(BOOL success, NSError *error) {
        if (success) {
            DLog(@"Saved CoreData context.");
        } else if (error) {
            DLog(@"Error saving context: %@", error.description);
        }
    }];
    
    return cdMeasuredPhoto;
}

#pragma mark - RCStereoDelegate

- (void) stereoDidUpdateProgress:(float)progress
{
    // Update modal view
    [progressView setProgress:progress];
}

- (void) stereoDidFinish
{
    [self handleStateEvent:EV_PROCESSING_FINISHED];
    [self gotoEditPhotoScreen];
}

#pragma mark -

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

#pragma mark - 3DK Stuff

- (void) startSensors
{
    LOGME
    [sensorDelegate startAllSensors];
}

- (void)stopSensors
{
    LOGME
    [sensorDelegate stopAllSensors];
}

- (void)startSensorFusion
{
    LOGME
    SENSOR_FUSION.delegate = self;
    [[sensorDelegate getVideoProvider] setDelegate:nil];
    [SENSOR_FUSION startSensorFusionWithDevice:[sensorDelegate getVideoDevice]];
}

- (void)stopSensorFusion
{
    LOGME
    [SENSOR_FUSION stopSensorFusion];
    [[sensorDelegate getVideoProvider] setDelegate:self.arView.videoView];
}

#pragma mark - RCSensorFusionDelegate

- (void) sensorFusionDidChangeStatus:(RCSensorFusionStatus *)status
{
    if ([status.error isKindOfClass:[RCSensorFusionError class]])
    {
        DLog(@"ERROR code %i", status.error.code);
        if(status.error.code == RCSensorFusionErrorCodeTooFast) {
            [self handleStateEvent:EV_FASTFAIL];
        } else if(status.error.code == RCSensorFusionErrorCodeOther) {
            [self handleStateEvent:EV_FAIL];
        } else if(status.error.code == RCSensorFusionErrorCodeVision) {
            [self handleStateEvent:EV_VISIONFAIL];
        }
    }
    [self updateProgress:status.progress];
    if(currentState == ST_INITIALIZING && status.runState == RCSensorFusionRunStateRunning) {
        [self handleStateEvent:EV_INITIALIZED];
    }
}

- (void) sensorFusionDidUpdateData:(RCSensorFusionData*)data
{
    goodPoints = [[NSMutableArray alloc] init];
    NSMutableArray *badPoints = [[NSMutableArray alloc] init];
    NSMutableArray *depths = [[NSMutableArray alloc] init];
    
    float mean;
    float sum = 0.;
    int count = 0;
    for(RCFeaturePoint *feature in data.featurePoints)
    {
        [depths addObject:[NSNumber numberWithFloat:feature.originalDepth.scalar]];
        sum += feature.originalDepth.scalar;
        count++;
        
        if((feature.originalDepth.standardDeviation / feature.originalDepth.scalar < .01 || feature.originalDepth.standardDeviation < .004) && feature.initialized)
        {
            [goodPoints addObject:feature];
        } else
        {
            [badPoints addObject:feature];
        }
    }
    
    mean = sum / count;

    NSNumber *median;
    if([depths count]) {
        NSArray *sorted = [depths sortedArrayUsingSelector:@selector(compare:)];
        long middle = [sorted count] / 2;
        median = [sorted objectAtIndex:middle];
    } else median = [NSNumber numberWithFloat:2.];

    if(data.sampleBuffer)
    {
        lastSensorFusionDataWithImage = data;
        
        [self.arView.videoView displaySampleBuffer:data.sampleBuffer];
        
        [self.arView.featuresLayer updateFeatures:goodPoints];
        [self.arView.initializingFeaturesLayer updateFeatures:badPoints];
        
        if(setups[currentState].stereo) [STEREO processFrame:data withFinal:false];
    }
    
    if (currentState == ST_MOVING) [instructionsView updateDotPosition:data.transformation withDepth:[median floatValue]];
}

/** delegate method of MPInstructionsViewDelegate. tells us when the dot has reached the edge of the circle. */
- (void) moveComplete
{
    [self handleStateEvent:EV_MOVE_DONE];
}

- (void)showProgressWithTitle:(NSString*)title
{
    [progressView setProgress:0];
    progressView.labelText = title;
    progressView.mode = MBProgressHUDModeAnnularDeterminate;
    [progressView show:YES];
}

- (void)showIndeterminateProgress:(NSString*)title
{
    progressView.labelText = title;
    progressView.mode = MBProgressHUDModeIndeterminate;
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
        
        if (hide) [self.messageLabel fadeOutWithDuration:2 andWait:5];
    }
    else
    {
        [self hideMessage];
    }
}

- (void)hideMessage
{
    [self.messageLabel fadeOutWithDuration:0.5 andWait:0];
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

@end
