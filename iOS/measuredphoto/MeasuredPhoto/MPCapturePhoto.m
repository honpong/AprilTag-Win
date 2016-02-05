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
#import "MPLoupe.h"
#import "MPLocalMoviePlayer.h"
#import "UIImage+MPImageFile.h"
#import "MPSurveyAnswer.h"
#import "MPDMeasuredPhoto.h"
#import "MPDLocation.h"
#import "CoreData+MagicalRecord.h"
#import "MBProgressHUD.h"
#import "MPDMeasuredPhoto+MPDMeasuredPhotoExt.h"
#import "MPEditPhoto.h"
#import "MPGalleryController.h"

static UIDeviceOrientation currentUIOrientation = UIDeviceOrientationPortrait;

typedef enum
{
    MOVING_START, MOVING_VERTICAL, MOVING_HORIZONTAL
} movement_status;

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

    RCSensorManager* sensorManager;
    
    MPDMeasuredPhoto* measuredPhoto;
    
    BOOL didGetVisionError;
    RCSensorFusionErrorCode lastErrorCode;
    
    movement_status moving_state;
}

@synthesize toolbar, galleryButton, shutterButton, messageLabel, questionLabel, questionSegButton, questionView, arView, containerView;

typedef NS_ENUM(int, AlertTag) {
    AlertTagTutorial = 0,
    AlertTagInstructions = 1
};

#pragma mark - State Machine

typedef enum
{
    BUTTON_SHUTTER, BUTTON_SHUTTER_DISABLED, BUTTON_DELETE, BUTTON_CANCEL, BUTTON_SHUTTER_ANIMATED
} ButtonImage;

typedef NS_ENUM(int, SpinnerType) {
    SpinnerTypeNone,
    SpinnerTypeDeterminate,
    SpinnerTypeIndeterminate
};

typedef NS_ENUM(int, MessageColor) {
    ColorGray,
    ColorYellow,
    ColorRed
};

enum state { ST_STARTUP, ST_READY, ST_INITIALIZING, ST_MOVING, ST_CAPTURE, ST_PROCESSING, ST_ERROR, ST_ROTATED_ERROR, ST_DISK_SPACE, ST_FINISHED, ST_ANY } currentState;
enum event { EV_RESUME, EV_FIRSTTIME, EV_VISIONFAIL, EV_FASTFAIL, EV_FAIL, EV_SHUTTER_TAP, EV_PAUSE, EV_CANCEL, EV_MOVE_DONE, EV_MOVE_UNDONE, EV_PROCESSING_FINISHED, EV_INITIALIZED, EV_STEREOFAIL, EV_DISK_SPACE, EV_ROTATEDFAIL };

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
    bool showGalleryButton;
    const char *title;
    MessageColor messageColor;
    const char *message;
} statesetup;

static statesetup setups[] =
{
    //                  button image               sensors fusion   shw-msmnts  badfeat  instrct ftrs    prgrs                     autohide stillPhoto  stereo  showGalleryButton title           messageColor                                       message
    { ST_STARTUP,       BUTTON_SHUTTER_DISABLED,   false,  false,   false,      false,   false,  false,  SpinnerTypeNone,          false,   false,      false,  true,             "Startup",      ColorGray,    "Loading" },
    { ST_READY,         BUTTON_SHUTTER,            true,   false,   false,      false,   false,  false,  SpinnerTypeNone,          true,    false,      false,  true,             "Ready",        ColorGray,    "Point the camera at the object you want to measure, then press the button." },
    { ST_INITIALIZING,  BUTTON_SHUTTER_DISABLED,   true,   true,    false,      true,    false,  true,   SpinnerTypeDeterminate,   true,    false,      false,  true,             "Initializing", ColorGray,    "Hold still" },
    { ST_MOVING,        BUTTON_DELETE,             true,   true,    false,      true,    true,   true,   SpinnerTypeNone,          false,   false,      true,   false,            "Moving",       ColorGray,    "Move in any direction until the arrow fills with green." },
    { ST_CAPTURE,       BUTTON_SHUTTER_ANIMATED,   true,   true,    false,      true,    true,   true,   SpinnerTypeNone,          false,   false,      true,   false,            "Capture",      ColorGray,    "Press the button to finish the photo." },
    { ST_PROCESSING,    BUTTON_SHUTTER_DISABLED,   false,  false,   false,      false,   false,  false,  SpinnerTypeDeterminate,   true,    true,       false,  false,            "Processing",   ColorGray,    "Please wait" },
    { ST_ERROR,         BUTTON_DELETE,             true,   false,   true,       false,   false,  false,  SpinnerTypeNone,          false,   false,      false,  true,             "Error",        ColorRed,     "Whoops, something went wrong. Try again." },
    { ST_ROTATED_ERROR, BUTTON_DELETE,             true,   false,   true,       false,   false,  false,  SpinnerTypeNone,          false,   false,      false,  true,             "Error",        ColorRed,     "You looked away from what you were measuring. Try again without turning." },
    { ST_DISK_SPACE,    BUTTON_SHUTTER_DISABLED,   true,   false,   true,       false,   false,  false,  SpinnerTypeNone,          false,   false,      false,  true,             "Error",        ColorRed,     "Your device is low on storage space. Free up some space first." },
    { ST_FINISHED,      BUTTON_DELETE,             false,  false,   true,       false,   false,  false,  SpinnerTypeNone,          true,    true,       false,  true,             "Finished",     ColorGray,    "" }
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
    { ST_MOVING, EV_ROTATEDFAIL, ST_ROTATED_ERROR },
    { ST_CAPTURE, EV_SHUTTER_TAP, ST_PROCESSING },
    { ST_CAPTURE, EV_MOVE_UNDONE, ST_MOVING },
    { ST_CAPTURE, EV_FAIL, ST_ERROR },
    { ST_CAPTURE, EV_FASTFAIL, ST_ERROR },
    { ST_CAPTURE, EV_ROTATEDFAIL, ST_ROTATED_ERROR },
    { ST_PROCESSING, EV_PROCESSING_FINISHED, ST_FINISHED },
    { ST_PROCESSING, EV_STEREOFAIL, ST_ERROR },
    { ST_ERROR, EV_SHUTTER_TAP, ST_READY },
    { ST_ROTATED_ERROR, EV_SHUTTER_TAP, ST_READY },
    { ST_FINISHED, EV_SHUTTER_TAP, ST_READY },
    { ST_FINISHED, EV_PAUSE, ST_FINISHED },
    { ST_ANY, EV_PAUSE, ST_STARTUP },
    { ST_ANY, EV_CANCEL, ST_STARTUP },
    { ST_ANY, EV_DISK_SPACE, ST_DISK_SPACE }
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
    if (![self handleStateTransition:newState]) return;

    if(!oldSetup.sensorCapture && newSetup.sensorCapture)
        [self startSensors];
    if(!oldSetup.sensorFusion && newSetup.sensorFusion)
        [self startSensorFusion];
    if(oldSetup.sensorCapture && !newSetup.sensorCapture)
        [self stopSensors];
    if(oldSetup.sensorFusion && !newSetup.sensorFusion)
        [self stopSensorFusion];
    if(oldSetup.features && !newSetup.features)
        [arView hideFeatures];
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
    if(!oldSetup.stereo && newSetup.stereo)
        [[RCStereo sharedInstance] reset];
    if (newSetup.progress == SpinnerTypeNone)
    {
        NSString* message = @(newSetup.message);
        [self showMessage:message withTitle:@"" withColor:newSetup.messageColor autoHide:newSetup.autohide];
    }
    else
    {
        [self hideMessage];
    }
    if(oldSetup.showSlideInstructions && !newSetup.showSlideInstructions)
        [self.arView.AROverlay setHidden:true];
    if(!oldSetup.showSlideInstructions && newSetup.showSlideInstructions)
        [self.arView.AROverlay setHidden:false];
    
    [self switchButtonImage:newSetup.buttonImage];
    
    if (!oldSetup.showGalleryButton && newSetup.showGalleryButton)
        self.galleryButton.enabled = YES;
    if (oldSetup.showGalleryButton && !newSetup.showGalleryButton)
        self.galleryButton.enabled = NO;
    
    lastTransitionTime = CACurrentMediaTime();
    currentState = newState;
}

/**
 @returns NO if transition should be canceled
 */
- (BOOL) handleStateTransition:(int)newState
{
    if (newState == ST_READY)
    {
        if (IS_DISK_SPACE_LOW)
        {
            [self handleStateEvent:EV_DISK_SPACE];
            return NO;
        }
        
        lastErrorCode = RCSensorFusionErrorCodeNone;
        didGetVisionError = NO;
    }
    else if (newState == ST_INITIALIZING)
    {
        [MPAnalytics startTimedEvent:@"MPhoto.Initializing" withParameters:nil];
    }
    else if(newState == ST_MOVING)
    {
        [MPAnalytics endTimedEvent:@"MPhoto.Initializing"];
        [self handleMoveStart];
    }
    else if(newState == ST_CAPTURE)
        [self handleMoveFinished];
    else if(newState == ST_PROCESSING)
    {
        if (IS_DISK_SPACE_LOW)
        {
            [self handleStateEvent:EV_DISK_SPACE];
            return NO;
        }
        else
        {
            [MPAnalytics startTimedEvent:@"MPhoto.StereoProcessing" withParameters:nil];
            [self handleCaptureFinished];
        }
    }
    else if (newState == ST_FINISHED)
    {
        [MPAnalytics endTimedEvent:@"MPhoto.StereoProcessing"];
        NSString* errorType = didGetVisionError ? @"Vision" : @"None";
        [MPAnalytics logEvent:@"MPhoto.Save" withParameters:@{ @"Error" : errorType }]; // successful
    }
    else if(currentState == ST_FINISHED && newState == ST_READY)
    {
        [self handlePhotoDeleted];
    }
    else if (newState == ST_ERROR)
    {
        if (lastErrorCode == RCSensorFusionErrorCodeTooFast)
            [MPAnalytics logEvent:@"MPhoto.Fail" withParameters:@{ @"Error" : @"TooFast", @"Vision Error Preceded" : didGetVisionError ? @"Yes" : @"No" }];
        else if (lastErrorCode == RCSensorFusionErrorCodeOther)
            [MPAnalytics logEvent:@"MPhoto.Fail" withParameters:@{ @"Error" : @"Other", @"Vision Error Preceded" : didGetVisionError ? @"Yes" : @"No" }];
        else
            [MPAnalytics logEvent:@"MPhoto.Fail" withParameters:@{ @"Error" : @"Unknown", @"Vision Error Preceded" : didGetVisionError ? @"Yes" : @"No" }];
    }
    
    return YES;
}

- (void)handleStateEvent:(int)event
{
    int newState = currentState;
    for(int i = 0; i < TRANS_COUNT; ++i) {
        if(transitions[i].state == currentState || transitions[i].state == ST_ANY) {
            if(transitions[i].event == event) {
                newState = transitions[i].newstate;
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
    
    sensorManager = [RCSensorManager sharedInstance];
    
    [self validateStateMachine];
    
    useLocation = [LOCATION_MANAGER isLocationExplicitlyAllowed] && [NSUserDefaults.standardUserDefaults boolForKey:PREF_USE_LOCATION];
    
    [[sensorManager getVideoProvider] addDelegate:self.arView.videoView];
    
    if (SYSTEM_VERSION_LESS_THAN(@"7")) questionSegButton.tintColor = [UIColor darkGrayColor];
    
    progressView = [[MBProgressHUD alloc] initWithView:self.containerView];
    progressView.mode = MBProgressHUDModeAnnularDeterminate;
    [self.containerView addSubview:progressView];
    
    [self.arView.AROverlay setHidden:true];
}

- (void)viewDidUnload
{
	LOGME
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
    [super viewDidAppear:animated];
    
    // don't do this here because it gets called when popping two VCs off the stack. instead call it when presenting from another VC.
//    [MPAnalytics logEvent:@"View.CapturePhoto"];
    
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

#pragma mark - Orientation

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
    if (currentUIOrientation != newOrientation && UIDeviceOrientationIsValidInterfaceOrientation(newOrientation))
    {
        currentUIOrientation = newOrientation;
        [self setOrientation:currentUIOrientation animated:YES];
    }
}

- (void) setOrientation:(UIDeviceOrientation)orientation animated:(BOOL)animated
{
    [self.view rotateChildViews:orientation animated:animated];
    MPOrientationChangeData* data = [MPOrientationChangeData dataWithOrientation:orientation animated:animated];
    [[NSNotificationCenter defaultCenter] postNotificationName:MPUIOrientationDidChangeNotification object:data];
}

#pragma mark -

- (void)handlePause
{
	LOGME
    if(currentState != ST_PROCESSING)
        [self handleStateEvent:EV_PAUSE];
}

- (void)handleResume
{
    LOGME
    
    if (useLocation) [LOCATION_MANAGER startLocationUpdates];
    
    if(currentState != ST_PROCESSING)
    {
        if ([RCDeviceInfo getFreeDiskSpaceInBytes] < 5000000)
        	[self handleStateEvent:EV_DISK_SPACE];
    	else
        	[self handleStateEvent:EV_RESUME];
    }
    
    [self handleOrientationChange]; // ensures that UI is in correct orientation
    if(currentState != ST_PROCESSING)
        [self.arView.videoView animateOpen:[MPCapturePhoto getCurrentUIOrientation]];
}

- (IBAction)handleShutterButton:(id)sender
{
    [self handleStateEvent:EV_SHUTTER_TAP];
}

- (IBAction)handleGalleryButton:(id)sender
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

- (void) handleMoveStart
{
    LOGME
    moving_state = MOVING_START;
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

    RCStereo * stereo = STEREO;
    stereo.delegate = self;
    [stereo setWorkingDirectory:WORKING_DIRECTORY_URL andGuid:measuredPhoto.id_guid andOrientation:currentUIOrientation];
    [stereo processFrame:lastSensorFusionDataWithImage withFinal:true];
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
        [measuredPhoto writeImagetoJpeg:lastSensorFusionDataWithImage.sampleBuffer withOrientation:stereo.orientation];
        // TODO: Handle potential stereo failure here (this function will return false)
        [stereo preprocess];
    });
}

- (void) gotoEditPhotoScreen
{
    [MPAnalytics logEvent:@"View.EditPhoto"];
    
    if ([self.presentingViewController isKindOfClass:[MPGalleryController class]])
    {
        MPGalleryController* galleryController = (MPGalleryController*)self.presentingViewController;
        MPEditPhoto* editPhotoController = galleryController.editPhotoController;
        editPhotoController.measuredPhoto = measuredPhoto;
        [self presentViewController:editPhotoController animated:YES completion:nil];
    }
    else if ([self.presentingViewController isKindOfClass:[MPEditPhoto class]])
    {
        MPEditPhoto* editPhotoController = (MPEditPhoto*)self.presentingViewController;
        editPhotoController.measuredPhoto = measuredPhoto;
        [self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
    }
}

- (void) gotoGallery
{
    [self.arView.videoView animateClosed:[MPCapturePhoto getCurrentUIOrientation] withCompletionBlock:^(BOOL finished) {
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
    [sensorManager startAllSensors];
}

- (void)stopSensors
{
    LOGME
    [sensorManager stopAllSensors];
}

- (void)startSensorFusion
{
    LOGME
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
    SENSOR_FUSION.delegate = self;
    [[sensorManager getVideoProvider] removeDelegate:self.arView.videoView];
    if (useLocation) [SENSOR_FUSION setLocation:[LOCATION_MANAGER getStoredLocation]];
    [SENSOR_FUSION startSensorFusionWithDevice:[sensorManager getVideoDevice]];
}

- (void)stopSensorFusion
{
    LOGME
    [SENSOR_FUSION stopSensorFusion];
    [[sensorManager getVideoProvider] addDelegate:self.arView.videoView];
    [[UIApplication sharedApplication] setIdleTimerDisabled:NO];
}

#pragma mark - RCSensorFusionDelegate

- (void) sensorFusionDidChangeStatus:(RCSensorFusionStatus *)status
{
    if ([status.error isKindOfClass:[RCSensorFusionError class]])
    {
        DLog(@"ERROR code %ld", status.error.code);
        lastErrorCode = (RCSensorFusionErrorCode)status.error.code;
        
        if(status.error.code == RCSensorFusionErrorCodeTooFast) {
            [self handleStateEvent:EV_FASTFAIL];
        } else if(status.error.code == RCSensorFusionErrorCodeOther) {
            [self handleStateEvent:EV_FAIL];
        } else if(status.error.code == RCSensorFusionErrorCodeVision) {
            didGetVisionError = YES;
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
    if(currentState == ST_INITIALIZING) [arView.AROverlay setInitialCamera:data.cameraTransformation];

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

    if (currentState == ST_MOVING || currentState == ST_CAPTURE)
    {
        //Compute the capture progress here
        float depth = [median floatValue];
        
        RCPoint *projectedpt = [[arView.AROverlay.initialCamera.rotation getInverse] transformPoint:[data.transformation.translation transformPoint:[[RCPoint alloc] initWithX:0. withY:0. withZ:0.]]];
        
        float targetDist = log(depth / 8. + 1.) + .05; // require movement of at least 5cm

        float dx = projectedpt.x / targetDist;
        float dy = projectedpt.y / targetDist;
        if(dx > 1.) dx = 1.;
        if(dx < -1.) dx = -1.;
        if(dy > 1.) dy = 1.;
        if(dy < -1.) dy = -1.;
        //hysteresis
        if(moving_state == MOVING_START)
        {
            moving_state = (fabs(dx) > fabs(dy)) ? MOVING_HORIZONTAL : MOVING_VERTICAL;
        }
        else if(moving_state == MOVING_HORIZONTAL)
        {
            if(fabs(dy) > fabs(dx) + .05) moving_state = MOVING_VERTICAL;
        }
        else if(moving_state == MOVING_VERTICAL)
        {
            if(fabs(dx) > fabs(dy) + .05) moving_state = MOVING_HORIZONTAL;
        }
        
        float progress;
        if(moving_state == MOVING_HORIZONTAL)
        {
            progress = fabs(dx);
            [arView.AROverlay setProgressHorizontal:dx withVertical:0.];
        }
        else
        {
            progress = fabs(dy);
            [arView.AROverlay setProgressHorizontal:0. withVertical:dy];
        }
        if(currentState == ST_MOVING && progress >= 1.) [self handleStateEvent:EV_MOVE_DONE];
        if(currentState == ST_CAPTURE && progress < 1.) [self handleStateEvent:EV_MOVE_UNDONE];
    }
    
    RCRotation *relative = [[arView.AROverlay.initialCamera.rotation getInverse] composeWithRotation:data.cameraTransformation.rotation];
    float theta = acos(relative.quaternionW) * 2.;
    if(theta > M_PI / 12.) [self showMessage:@"Don't turn. Move up, down, left, or right until the arrow fills with green." withTitle:@"Warning" withColor:ColorYellow autoHide:false];
    if(theta > M_PI / 6.) [self handleStateEvent:EV_ROTATEDFAIL];

    if(data.sampleBuffer)
    {
        lastSensorFusionDataWithImage = data;
        
        [self.arView.videoView displaySensorFusionData:data];
        
        [self.arView.featuresLayer updateFeatures:goodPoints];
        [self.arView.initializingFeaturesLayer updateFeatures:badPoints];
        
        if(setups[currentState].stereo) [STEREO processFrame:data withFinal:false];
    }
}

#pragma mark -

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

- (void)showMessage:(NSString*)message withTitle:(NSString*)title withColor:(MessageColor)color autoHide:(BOOL)hide
{
    if (message && message.length > 0)
    {
        self.messageLabel.hidden = NO;
        self.messageLabel.alpha = 1;
        self.messageLabel.text = message ? message : @"";
        
        if (color == ColorRed)
            self.messageLabel.backgroundColor = [UIColor colorWithRed:1 green:0 blue:0 alpha:.8];
        else if (color == ColorYellow)
            self.messageLabel.backgroundColor = [UIColor colorWithRed:.7 green:.6 blue:0 alpha:.9];
        else
            self.messageLabel.backgroundColor = [UIColor colorWithRed:0 green:0 blue:0 alpha:.3];
        
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
    
    [self.expandingCircleView stopHighlightAnimation];
    
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
            
        case BUTTON_SHUTTER_ANIMATED:
            imageName = @"PLCameraFloatingShutterButton";
            shutterButton.alpha = 1.;
            shutterButton.enabled = YES;
            [self.expandingCircleView startHighlightAnimation];
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
