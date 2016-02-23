//
//  TMViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMNewMeasurementVC.h"
#import "TMLocalMoviePlayer.h"
#import "TMLocationIntro.h"
#import "TMHistoryVC.h"
#import "RCCore/RCGeocoder.h"
#import "TMAnalytics.h"

@implementation TMNewMeasurementVC
{
    TMMeasurement *newMeasurement;
    TMLocation *locationObj;
    
    BOOL useLocation;
    
    MBProgressHUD *progressView;
    
    double lastTransitionTime;
    double lastFailTime;
    int filterStatusCode;
    
    RCSensorManager*sensorManager;
    
    RCTipView* tipsView;
    
    UIColor* originalDistanceTextColor;
    
    BOOL didGetVisionError;
    bool isCurrentFail;
}

#pragma mark - State Machine

static const double stateTimeout = 6.1;
//static const double failTimeout = 2.;

// obsolete, but keeping it around in case we need it later
typedef enum
{
    ICON_HIDDEN, ICON_RED, ICON_YELLOW, ICON_GREEN
} IconType;

// order is significant
enum state { ST_STARTUP, ST_READY, ST_INITIALIZING, ST_MEASURE, ST_FINISHED, ST_INIT_VISION_FAIL, ST_VISION_FAIL, ST_FASTFAIL, ST_FAIL, ST_ANY } currentState;
enum event { EV_RESUME, EV_CONVERGED, EV_STEADY_TIMEOUT, EV_VISIONFAIL, EV_FASTFAIL, EV_FAIL, EV_NOFAIL, EV_FAIL_EXPIRED, EV_TAP, EV_PAUSE, EV_CANCEL, EV_INITIALIZED };

typedef struct { enum state state; enum event event; enum state newstate; } transition;

typedef struct
{
    enum state state;
    IconType icon; // unused
    bool sensorFusion;
    bool sensorCapture;
    bool measuring;
    bool listBtnEnabled;
    bool retryBtnEnabled;
    bool showTape;
    bool showDistance;
    bool showTips;
    bool features;
    bool progress;
    const char *title;
    const char *message;
    bool autohide;
} statesetup;

static statesetup setups[] =
{
    //                                    fusion  sensors measure listBtn rtryBtn shwdstc shwtape showTips ftrs    prgrs   title           message         autohide
    { ST_STARTUP,           ICON_GREEN,   false,  false,  false,  true,   false,  false,  false,  false,   false,  false,  "",             "",             false},
    { ST_READY,             ICON_GREEN,   false,  true,   false,  true,   false,  false,  false,  true,    false,  false,  "Instructions", "Hold the device steady where you want to start the measurement and tap the screen to begin.", false },
    { ST_INITIALIZING,      ICON_GREEN,   true,   true,   false,  true,   true,   true,   false,  true,    true,   true,   "Hold still",   "Hold the device steady.", false},
    { ST_MEASURE,           ICON_GREEN,   true,   true,   true,   true,   true,   true,   true,   false,   true,   false,  "Measuring",    "Go! Move the device to the end of your measurement, and tap the screen to finish.", false },
    { ST_FINISHED,          ICON_GREEN,   false,  false,  false,  true,   true,   true,   true,   false,   false,  false,  "",             "", false },
    { ST_INIT_VISION_FAIL,  ICON_RED,     false,  true,   false,  true,   true,   true,   false,  false,   false,  false,  "Try again",    "The camera can't see well enough to measure. Try again with the camera pointed in a different direction.", false },
    { ST_VISION_FAIL,       ICON_RED,     true,   true,   true,   true,   true,   true,   true,   false,   true,   false,  "Warning",      "The camera can't see well enough to measure. Try pointing the camera in a different direction.", false },
    { ST_FASTFAIL,          ICON_RED,     false,  true,   false,  true,   true,   true,   true,   true,    false,  false,  "Try again",    "Sorry, that didn't work. For best results, move at a normal walking pace.", false },
    { ST_FAIL,              ICON_RED,     false,  true,   false,  true,   true,   true,   true,   true,    false,  false,  "Try again",    "Sorry, we need to try that again.", false },
};

static transition transitions[] =
{
    { ST_STARTUP, EV_RESUME, ST_READY },
    { ST_READY, EV_TAP, ST_INITIALIZING },
    { ST_INITIALIZING, EV_INITIALIZED, ST_MEASURE },
    { ST_INITIALIZING, EV_VISIONFAIL, ST_INIT_VISION_FAIL }, // don't quit on vision failure
    { ST_MEASURE, EV_TAP, ST_FINISHED },
    { ST_MEASURE, EV_FASTFAIL, ST_FASTFAIL },
    { ST_MEASURE, EV_FAIL, ST_FAIL },
    { ST_MEASURE, EV_VISIONFAIL, ST_VISION_FAIL },
    { ST_VISION_FAIL, EV_TAP, ST_FINISHED },
    { ST_VISION_FAIL, EV_FASTFAIL, ST_FASTFAIL },
    { ST_VISION_FAIL, EV_FAIL, ST_FAIL },
    { ST_VISION_FAIL, EV_NOFAIL, ST_MEASURE },
    { ST_INIT_VISION_FAIL, EV_FAIL_EXPIRED, ST_READY },
    { ST_VISION_FAIL, EV_FAIL_EXPIRED, ST_MEASURE },
    { ST_FASTFAIL, EV_FAIL_EXPIRED, ST_READY },
    { ST_FAIL, EV_FAIL_EXPIRED, ST_READY },
    { ST_INIT_VISION_FAIL, EV_TAP, ST_READY },
    { ST_FASTFAIL, EV_TAP, ST_READY },
    { ST_FAIL, EV_TAP, ST_READY },
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

    if(!oldSetup.sensorCapture && newSetup.sensorCapture)
        [self startSensors];
    if(!oldSetup.sensorFusion && newSetup.sensorFusion)
        [self startSensorFusion];
    if(!oldSetup.retryBtnEnabled && newSetup.retryBtnEnabled)
        self.btnRetry.enabled = YES;
    if(oldSetup.retryBtnEnabled && !newSetup.retryBtnEnabled)
        self.btnRetry.enabled = NO;
    if(!oldSetup.listBtnEnabled && newSetup.listBtnEnabled)
        self.btnList.enabled = YES;
    if(oldSetup.listBtnEnabled && !newSetup.listBtnEnabled)
        self.btnList.enabled = NO;
    if(!oldSetup.measuring && newSetup.measuring)
        [self startMeasuring];
    if(oldSetup.measuring && !newSetup.measuring)
        [self stopMeasuring];
    if(oldSetup.sensorCapture && !newSetup.sensorCapture)
        [self stopSensors];
    if(oldSetup.sensorFusion && !newSetup.sensorFusion)
        [self stopSensorFusion];
    if(!oldSetup.showDistance && newSetup.showDistance)
        [self show2dTape];
    if(oldSetup.showDistance && !newSetup.showDistance)
        [self hide2dTape];
    if(oldSetup.features && !newSetup.features)
        [self.arView hideFeatures];
    if(!oldSetup.features && newSetup.features)
        [self.arView showFeatures];
    if(oldSetup.progress && !newSetup.progress)
        [self hideProgress];
    if(!oldSetup.progress && newSetup.progress)
        [self showProgressWithTitle:@(newSetup.title)];
    if(!oldSetup.showTips && newSetup.showTips)
        [self showTipsView];
    if(oldSetup.showTips && !newSetup.showTips)
        [tipsView fadeOutWithDuration:.5 andWait:0];
    if(!oldSetup.sensorCapture && newSetup.sensorCapture)
        [self.arView.videoView animateOpen:UIDeviceOrientationPortrait];
    if(oldSetup.icon == ICON_GREEN && newSetup.icon == ICON_RED)
        self.distanceLabel.textColor = [UIColor redColor];
    if(oldSetup.icon == ICON_RED && newSetup.icon == ICON_GREEN)
        self.distanceLabel.textColor = originalDistanceTextColor;
    
    currentState = newState;
    
//    [self showIcon:newSetup.icon];
    
    NSString *message = [NSString stringWithFormat:@(newSetup.message), filterStatusCode];
    [self showMessage:message withTitle:@(newSetup.title) autoHide:newSetup.autohide];
    
    lastTransitionTime = CACurrentMediaTime();
    
    [self handleStateTransition:newState];
}

- (void) handleStateTransition:(int)newState
{
    if (newState == ST_READY)
    {
        didGetVisionError = NO;
    }
    else if (newState == ST_INITIALIZING)
    {
        [TMAnalytics startTimedEvent:@"Measurement.Initializing" withParameters:nil];
    }
    else if(newState == ST_MEASURE)
    {
        [TMAnalytics endTimedEvent:@"Measurement.Initializing"];
    }
    else if (newState == ST_FINISHED)
    {
        NSString* errorType = didGetVisionError ? @"Vision" : @"None";
        [TMAnalytics logEvent:@"Measurement.Save" withParameters:@{ @"Error" : errorType, @"Distance": [newMeasurement getDistRangeString] }]; // successful measurement
        [self saveAndGotoResult];
    }
    else if (newState == ST_FASTFAIL)
    {
        [TMAnalytics logEvent:@"Measurement.Fail" withParameters:@{ @"Error" : @"TooFast", @"Vision Error Preceded" : didGetVisionError ? @"Yes" : @"No" }];
    }
    else if (newState == ST_FAIL)
    {
        [TMAnalytics logEvent:@"Measurement.Fail" withParameters:@{ @"Error" : @"Other", @"Vision Error Preceded" : didGetVisionError ? @"Yes" : @"No" }];
    }
    else if (newState == ST_INIT_VISION_FAIL)
    {
        [TMAnalytics logEvent:@"Measurement.Fail" withParameters:@{ @"Error" : @"InitVision" }];
    }
    else if (newState == ST_VISION_FAIL)
    {
        didGetVisionError = YES;
    }
}

- (void)handleStateEvent:(int)event
{
    DLog(@"State event %i", event);
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
    
    [self validateStateMachine];
    
    sensorManager = [RCSensorManager sharedInstance];
    
    //setup screen tap detection
    UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapGesture:)];
    tapGesture.numberOfTapsRequired = 1;
    [self.arView addGestureRecognizer:tapGesture];
    
    if (newMeasurement.units == UnitsImperial) self.distanceLabel.centerAlignmentExcludesFraction = YES;
    
    [[sensorManager getVideoProvider] addDelegate:self.arView.videoView];
    
    progressView = [[MBProgressHUD alloc] initWithView:self.navigationController.view];
    [self.view addSubview:progressView];
    
    tipsView = [RCTipView new];
    [self.view addSubview:tipsView];
    [tipsView addCenterXInSuperviewConstraints];
    [tipsView addWidthConstraint:280 andHeightConstraint:80];
    [tipsView addBottomSpaceToSuperviewConstraint:20];
    tipsView.alpha = 0;
    tipsView.delegate = self;
    tipsView.tips = [self buildTipsArray];
    
    self.distanceLabel.textAlignment = NSTextAlignmentCenter;
    self.distanceLabel.centerAlignmentExcludesFraction = YES;
    CGFloat fontSize = UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad ? 70 : 50;
    self.distanceLabel.font = [UIFont systemFontOfSize:fontSize];
    self.distanceLabel.textColor = COLOR_DULL_YELLOW;
    self.distanceLabel.shadowColor = [UIColor darkGrayColor];
    originalDistanceTextColor = self.distanceLabel.textColor;
    
    [self.tapeView2D drawTickMarksWithUnits:(Units)[[NSUserDefaults standardUserDefaults] integerForKey:PREF_UNITS]];
    self.tapeView2D.transform = CGAffineTransformMakeRotation((float)M_PI_2);
    self.tapeView2D.layer.sublayerTransform = [self get3DTransform];
}

- (CATransform3D) get3DTransform
{
    CGFloat scale = .7;
    CATransform3D rotationAndPerspectiveTransform = CATransform3DIdentity;
    rotationAndPerspectiveTransform.m34 = 1.0 / -100;
    rotationAndPerspectiveTransform = CATransform3DRotate(rotationAndPerspectiveTransform, 65.0f * -M_PI / 180.0f, 0.0f, 1.0f, 0.0f);
    rotationAndPerspectiveTransform = CATransform3DScale(rotationAndPerspectiveTransform, scale, scale, scale);
    return rotationAndPerspectiveTransform;
}

- (void)viewDidUnload
{
	LOGME
	[self setDistanceLabel:nil];
	[self setLblInstructions:nil];
    [self setArView:nil];
    [self setInstructionsBg:nil];
    [self setTapeView2D:nil];
    [self setBtnRetry:nil];
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
    [TMAnalytics logEvent:@"View.NewMeasurement"];
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
    
    newMeasurement = [TMMeasurement getNewMeasurement];
    newMeasurement.type = self.type;
    [newMeasurement autoSelectUnitsScale];
    
    [self handleResume];
    SENSOR_FUSION.delegate = self;
}

- (void) moviePlayerWillExitFullScreen:(NSNotification*)notification
{
    [self dismissMoviePlayerViewControllerAnimated];
}

- (void) viewWillDisappear:(BOOL)animated
{
    LOGME
    SENSOR_FUSION.delegate = nil;
    [super viewWillDisappear:animated];
    [self handleStateEvent:EV_CANCEL];
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    self.navigationController.navigationBar.topItem.title = @""; // so that it doesn't show wrong title when we go back to measurement type screen
}

- (void) viewDidDisappear:(BOOL)animated
{
    [super viewDidDisappear:animated];
}

- (void) willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
    [UIView setAnimationsEnabled:NO]; //disable weird rotation animation on video preview
    [super willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
}

- (void) didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
    [super didRotateFromInterfaceOrientation:fromInterfaceOrientation];
    [UIView setAnimationsEnabled:YES];
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
    
    useLocation = [LOCATION_MANAGER isLocationExplicitlyAllowed] && [NSUserDefaults.standardUserDefaults boolForKey:PREF_USE_LOCATION];
    if (useLocation) [self updateLocation];
}

- (void) handleTapGesture:(UIGestureRecognizer *) sender
{
    if (sender.state != UIGestureRecognizerStateEnded) return;
    [self handleStateEvent:EV_TAP];
}

- (IBAction)handleListButton:(id)sender
{
    [self hide2dTape];
    [self.arView hideFeatures];
    self.distanceLabel.hidden = YES;
    [tipsView fadeOutWithDuration:.2 andWait:0];
    
    #if TARGET_IPHONE_SIMULATOR
    [self gotoHistory]; // skip CRT animation. doesn't work on simulator.
    #else
    __weak TMNewMeasurementVC* weakSelf = self;
    [self.arView.videoView animateClosed:UIDeviceOrientationPortrait withCompletionBlock:^(BOOL finished) {
        [weakSelf gotoHistory];
    }];
    #endif
}

- (void) gotoHistory
{
    TMHistoryVC* vc = [self.navigationController.storyboard instantiateViewControllerWithIdentifier:@"History"];
    [self.navigationController pushViewController:vc animated:NO];
}

- (void) updateLocation
{
    LOCATION_MANAGER.delegate = self;
    [LOCATION_MANAGER startLocationUpdates];
}

#pragma mark - CLLocationManagerDelegate

- (void) locationManager:(CLLocationManager *)manager didUpdateLocations:(NSArray *)locations
{
    LOCATION_MANAGER.delegate = nil;
    
    CLLocation *clLocation = [LOCATION_MANAGER getStoredLocation];
    [SENSOR_FUSION setLocation:clLocation];
    
    if([NSUserDefaults.standardUserDefaults boolForKey:PREF_ADD_LOCATION] && clLocation)
    {
        locationObj = [TMLocation getLocationNear:clLocation];
        
        if (locationObj == nil)
        {
            locationObj = (TMLocation*)[TMLocation getNewLocation];
            locationObj.latititude = clLocation.coordinate.latitude;
            locationObj.longitude = clLocation.coordinate.longitude;
            locationObj.accuracyInMeters = clLocation.horizontalAccuracy;
            locationObj.timestamp = [[NSDate date] timeIntervalSince1970];
            locationObj.syncPending = YES;
            [locationObj insertIntoDb];
        }
        
        if (locationObj.address == nil)
        {
            __block NSString *block_address = nil;
            [RCGeocoder reverseGeocodeLocation:[LOCATION_MANAGER getStoredLocation] withCompletionBlock:^(NSString *address, NSError *error)
             {
                 block_address = address;
                 if(block_address) locationObj.address = block_address;
             }];
        }
        
        DLog(@"Added location to measurement");
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

- (void)startMeasuring
{
    LOGME
    [TMAnalytics startTimedEvent:@"Measurement.New" withParameters:nil];
}

- (void)stopMeasuring
{
    LOGME
    [TMAnalytics endTimedEvent:@"Measurement.New"];
}

- (void)startSensorFusion
{
    LOGME
    [[UIApplication sharedApplication] setIdleTimerDisabled:YES];
    [[sensorManager getVideoProvider] removeDelegate:self.arView.videoView];
    if (SENSOR_FUSION.location == nil) DLog(@"WARNING: No location is set for sensor fusion");
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
    double currentTime = CACurrentMediaTime();
    
    if ([status.error isKindOfClass:[RCSensorFusionError class]])
    {
        if(status.error.code == RCSensorFusionErrorCodeTooFast) {
            [self handleStateEvent:EV_FASTFAIL];
        } else if(status.error.code == RCSensorFusionErrorCodeOther) {
            [self handleStateEvent:EV_FAIL];
        } else if(status.error.code == RCSensorFusionErrorCodeVision) {
            [self handleStateEvent:EV_VISIONFAIL];
        }
        lastFailTime = currentTime;
        isCurrentFail = true;
    }
    else
    {
        isCurrentFail = false;
    }
    if(currentState == ST_INITIALIZING)
    {
        if (status.runState == RCSensorFusionRunStateRunning)
            [self handleStateEvent:EV_INITIALIZED];
        else
        {
            [self updateProgress:status.progress];
        }
    }
}

- (void) sensorFusionDidUpdateData:(RCSensorFusionData*)data
{
    if (setups[currentState].measuring) [self updateMeasurement:data.transformation withTotalPath:data.totalPathLength];

    if(data.sampleBuffer)
    {
        [self.arView.videoView receiveVideoFrame:data.sampleBuffer];
        [self.arView.featuresLayer updateFeatures:data.featurePoints];
    }
    
    if (!isCurrentFail && lastFailTime != 0 && CACurrentMediaTime() - lastFailTime > 1)
    {
        [self handleStateEvent:EV_FAIL_EXPIRED];
        lastFailTime = 0;
    }
}

#pragma mark - Misc

- (void) updateMeasurement:(RCTransformation*)transformation withTotalPath:(RCScalar *)totalPath
{
    newMeasurement.xDisp = transformation.translation.x;
    newMeasurement.xDisp_stdev = transformation.translation.stdx;
    newMeasurement.yDisp = transformation.translation.y;
    newMeasurement.yDisp_stdev = transformation.translation.stdy;
    newMeasurement.zDisp = transformation.translation.z;
    newMeasurement.zDisp_stdev = transformation.translation.stdz;
    newMeasurement.totalPath = totalPath.scalar;
    newMeasurement.totalPath_stdev = totalPath.standardDeviation;
    float ptdist = sqrt(transformation.translation.x*transformation.translation.x + transformation.translation.y*transformation.translation.y + transformation.translation.z*transformation.translation.z);
    newMeasurement.pointToPoint = ptdist;
    float hdist = sqrt(transformation.translation.x*transformation.translation.x + transformation.translation.y*transformation.translation.y);
    newMeasurement.horzDist = hdist;
    float hxlin = transformation.translation.x / hdist * transformation.translation.stdx, hylin = transformation.translation.y / hdist * transformation.translation.stdy;
    newMeasurement.horzDist_stdev = sqrt(hxlin * hxlin + hylin * hylin);
    float ptxlin = transformation.translation.x / ptdist * transformation.translation.stdx, ptylin = transformation.translation.y / ptdist * transformation.translation.stdy, ptzlin = transformation.translation.z / ptdist * transformation.translation.stdz;
    newMeasurement.pointToPoint_stdev = sqrt(ptxlin * ptxlin + ptylin * ptylin + ptzlin * ptzlin);
    
    //TODO: Store rotation if needed - update to quaternion
    
    [newMeasurement autoSelectUnitsScale];
    
    [self updateDistanceLabel];
    [self.tapeView2D moveTapeWithDistance:[newMeasurement getPrimaryMeasurementDist] withUnits:newMeasurement.units];
}

- (void)saveMeasurement
{
    LOGME
    newMeasurement.type = self.type;
    newMeasurement.timestamp = [[NSDate date] timeIntervalSince1970];
    newMeasurement.syncPending = YES;
    
    [newMeasurement insertIntoDb]; //order is important. this must be inserted before location is added.
    
    if ([NSUserDefaults.standardUserDefaults boolForKey:PREF_USE_LOCATION] && [NSUserDefaults.standardUserDefaults boolForKey:PREF_ADD_LOCATION])
    {
        [locationObj addMeasurementObject:newMeasurement];
    }
    
    [DATA_MANAGER saveContext];
    
//    if (locationObj.syncPending && [USER_MANAGER getLoginState] == LoginStateYes)
//    {
//        __weak TMNewMeasurementVC* weakSelf = self;
//        [locationObj
//         postToServer:^(NSInteger transId)
//         {
//             DLog(@"Post location success callback");
//             locationObj.syncPending = NO;
//             [DATA_MANAGER saveContext];
//             [weakSelf postMeasurement];
//         }
//         onFailure:^(NSInteger statusCode)
//         {
//             DLog(@"Post location failure callback");
//         }
//         ];
//    }
//    else
//    {
////        [self postMeasurement];
//    }
}

- (void)showProgressWithTitle:(NSString*)title
{
    progressView.mode = MBProgressHUDModeAnnularDeterminate;
    progressView.labelText = title;
    [progressView show:YES];
}

- (void) showIndeterminateProgress:(NSString*)title
{
    progressView.mode = MBProgressHUDModeIndeterminate;
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

-(void)postMeasurement
{
    [newMeasurement
     postToServer:
     ^(NSInteger transId)
     {
         DLog(@"postMeasurement success callback");
         newMeasurement.syncPending = NO;
         [DATA_MANAGER saveContext];
     }
     onFailure:
     ^(NSInteger statusCode)
     {
         //TODO: handle error
         DLog(@"Post measurement failure callback");
     }
     ];
}

- (void)showMessage:(NSString*)message withTitle:(NSString*)title autoHide:(BOOL)hide
{
    self.instructionsBg.hidden = NO;
    self.lblInstructions.hidden = NO;
    
    self.lblInstructions.text = message ? message : @"";
    self.navigationController.navigationBar.topItem.title = title ? title : @"";
    
    self.instructionsBg.alpha = 1.;
    self.lblInstructions.alpha = 1.;
    
    if (hide)
    {
        int const delayTime = 5;
        int const fadeTime = 2;
        
        [self.lblInstructions fadeOutWithDuration:fadeTime andWait:delayTime];
        [self.instructionsBg fadeOutWithDuration:fadeTime andWait:delayTime];
    }
}

- (void)hideMessage
{
    [self.lblInstructions fadeOutWithDuration:0.5 andWait:0];
    [self.instructionsBg fadeOutWithDuration:0.5 andWait:0];
    
    self.navigationController.navigationBar.topItem.title = @"";
}

- (void)show2dTape
{
    self.tapeView2D.hidden = NO;
    self.distanceLabel.hidden = NO;
}

- (void)hide2dTape
{
    self.tapeView2D.hidden = YES;
    self.distanceLabel.hidden = YES;
}

- (void)updateDistanceLabel
{
    [self.distanceLabel setDistance:[newMeasurement getPrimaryDistanceObject]];
}

- (IBAction)handleRetryButton:(id)sender
{
    [TMAnalytics logEvent:@"View.NewMeasurement.Reset"];
    [self transitionToState:ST_READY];
}

- (void) saveAndGotoResult
{
    [self saveMeasurement];
    TMResultsVC* resultsVC = [self.navigationController.storyboard instantiateViewControllerWithIdentifier:@"Results"];
    resultsVC.theMeasurement = newMeasurement;
    [self.navigationController pushViewController:resultsVC animated:YES];
}

- (void) showTipsView
{
    NSInteger tipIndex = [NSUserDefaults.standardUserDefaults integerForKey:PREF_LAST_TIP_INDEX];
    [tipsView showTip:tipIndex + 1];
    [tipsView fadeInWithDuration:.5 andWait:0];
}

- (NSArray*) buildTipsArray
{
    return @[@"This app uses the camera to \"see\" how far the device moved. Don't block the camera.",
             @"The camera can see well when small blue dots appear on the screen while measuring.",
             @"The app shows the straight-line distance your device moves from the start to the end point.",
             @"Hold the device steady with two hands to minimize small vibrations.",
             @"Don't move too fast or too slow. Normal walking speed works well.",
             @"To measure small objects, move the device from one end of the object to the other.",
             @"To measure long distances, hold the device in front of you as you walk.",
             @"If you have problems, try again with the camera pointed in a different direction.",
             @"There is no limit to how far you can measure with Endless Tape Measure."];
}

#pragma mark - RCTipViewDelegate

- (void) tipIndexUpdated:(int)index
{
    [NSUserDefaults.standardUserDefaults setInteger:index forKey:PREF_LAST_TIP_INDEX];
}

@end
