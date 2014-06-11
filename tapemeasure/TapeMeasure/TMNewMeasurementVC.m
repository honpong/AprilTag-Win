//
//  TMViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMNewMeasurementVC.h"
#import <RCCore/RCCore.h>
#import <RC3DK/RC3DK.h>

@implementation TMNewMeasurementVC
{
    TMMeasurement *newMeasurement;
    
    BOOL useLocation;
    
    MBProgressHUD *progressView;
    
    double lastTransitionTime;
    double lastFailTime;
    int filterStatusCode;
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
enum state { ST_STARTUP, ST_READY, ST_INITIALIZING, ST_MEASURE, ST_FINISHED, ST_VISIONFAIL, ST_FASTFAIL, ST_FAIL, ST_ANY } currentState;
enum event { EV_RESUME, EV_CONVERGED, EV_STEADY_TIMEOUT, EV_VISIONFAIL, EV_FASTFAIL, EV_FAIL, EV_FAIL_EXPIRED, EV_TAP, EV_PAUSE, EV_CANCEL, EV_INITIALIZED };

typedef struct { enum state state; enum event event; enum state newstate; } transition;

typedef struct
{
    enum state state;
    IconType icon; // unused
    bool motionCap;
    bool videoCapture;
    bool avSession; // unused
    bool measuring;
    bool listBtnEnabled;
    bool retryBtnEnabled;
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
    //                                moCap   vidCap  session measure listBtn rtryBtn shwdstc shwtape ftrs    prgrs   title           message         autohide
    { ST_STARTUP,       ICON_GREEN,   false,  false,  false,  false,  true,   false,  false,  false,  false,  false,  "Starting Up",  "Please wait", false},
    { ST_READY,         ICON_GREEN,   true,   false,  true,   false,  true,   false,  false,  false,  false,  false,  "Instructions", "Stand where you want to start the measurement, point the camera forward, and tap the screen to initalize.", false },
    { ST_INITIALIZING,  ICON_GREEN,   true,   true,   true,   false,  true,   true,   true,   false,  true,   true,   "Hold still",   "Hold the device still and keep it pointed forward.", false},
    { ST_MEASURE,       ICON_GREEN,   true,   true,   true,   true,   true,   true,   true,   true,   true,   false,  "Measuring",    "Go! Move to the place where you want to end your measurement, then tap the screen to finish. Keep the camera pointed forward.", false },
    { ST_FINISHED,      ICON_GREEN,   false,  false,  false,  false,  true,   true,   true,   true,   false,  false,  "Finished",     "Looks good. Press save to name and store your measurement.", false },
    { ST_VISIONFAIL,    ICON_RED,     true,   true,   true,   false,  true,   true,   true,   true,   false,  false,  "Try again",    "Sorry, the camera can't see well enough to measure right now. Try to keep some blue dots in sight, and make sure the area is well lit.", false },
    { ST_FASTFAIL,      ICON_RED,     true,   true,   true,   false,  true,   true,   true,   true,   false,  false,  "Try again",    "Sorry, that didn't work. For best results, move at a normal walking pace.", false },
    { ST_FAIL,          ICON_RED,     true,   true,   true,   false,  true,   true,   true,   true,   false,  false,  "Try again",    "Sorry, we need to try that again.", false },
};

static transition transitions[] =
{
    { ST_STARTUP, EV_RESUME, ST_READY },
    { ST_READY, EV_TAP, ST_INITIALIZING },
    { ST_INITIALIZING, EV_INITIALIZED, ST_MEASURE },
//    { ST_INITIALIZING, EV_VISIONFAIL, ST_VISIONFAIL }, // don't quit on vision failure
    { ST_MEASURE, EV_TAP, ST_FINISHED },
    { ST_MEASURE, EV_FASTFAIL, ST_FASTFAIL },
    { ST_MEASURE, EV_FAIL, ST_FAIL },
//    { ST_MEASURE, EV_VISIONFAIL, ST_VISIONFAIL }, // don't quit on vision failure
    { ST_VISIONFAIL, EV_FAIL_EXPIRED, ST_READY },
    { ST_FASTFAIL, EV_FAIL_EXPIRED, ST_READY },
    { ST_FAIL, EV_FAIL_EXPIRED, ST_READY },
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
    if(!oldSetup.motionCap && newSetup.motionCap)
        [self startMotion];
    if(!oldSetup.videoCapture && newSetup.videoCapture)
        [self startVideoCapture];
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
    if(oldSetup.motionCap && !newSetup.motionCap)
        [self stopMotion];
    if(oldSetup.videoCapture && !newSetup.videoCapture)
        [self stopVideoCapture];
    if(oldSetup.avSession && !newSetup.avSession)
        [SESSION_MANAGER endSession];
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
    
    currentState = newState;
    
//    [self showIcon:newSetup.icon];
    
    NSString *message = [NSString stringWithFormat:@(newSetup.message), filterStatusCode];
    [self showMessage:message withTitle:@(newSetup.title) autoHide:newSetup.autohide];
    
    lastTransitionTime = CACurrentMediaTime();
    
    if (currentState == ST_FINISHED) [self saveAndGotoResult];
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
    
    useLocation = [LOCATION_MANAGER isLocationAuthorized] && [[NSUserDefaults standardUserDefaults] boolForKey:PREF_ADD_LOCATION];
    
    //setup screen tap detection
    UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapGesture:)];
    tapGesture.numberOfTapsRequired = 1;
    [self.arView addGestureRecognizer:tapGesture];
    
    self.distanceLabel.centerAlignmentExcludesFraction = YES;
    
    [VIDEO_MANAGER setDelegate:self.arView.videoView];
    
    progressView = [[MBProgressHUD alloc] initWithView:self.navigationController.view];
    [self.view addSubview:progressView];
    
    [self setupDataCapture];
}

- (void) viewDidLayoutSubviews
{
    [self.tapeView2D drawTickMarksWithUnits:(Units)[[NSUserDefaults standardUserDefaults] integerForKey:PREF_UNITS]];
    self.tapeView2D.transform = CGAffineTransformMakeRotation((float)M_PI_2);
    self.tapeView2D.layer.sublayerTransform = [self get3DTransform];
//    self.distanceLabel.transform = CGAffineTransformMakeScale(1.5, 1.5);
    [self.view layoutSubviews];
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
    [self handleResume];
    SENSOR_FUSION.delegate = self;
}

- (void) viewWillDisappear:(BOOL)animated
{
    LOGME
    SENSOR_FUSION.delegate = nil;
    [super viewWillDisappear:animated];
    [self handleStateEvent:EV_CANCEL];
    [self endAVSessionInBackground];
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
}

- (void) handleTapGesture:(UIGestureRecognizer *) sender
{
    if (sender.state != UIGestureRecognizerStateEnded) return;
    [self handleStateEvent:EV_TAP];
}

#pragma mark - 3DK Stuff

- (void) setupDataCapture
{
    LOGME
    
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        /** Expensive. Can cause UI to lag if called at the wrong time. */
        [VIDEO_MANAGER setupWithSession:SESSION_MANAGER.session];
    });
}

- (void) startMotion
{
    LOGME
    if (![MOTION_MANAGER isCapturing]) [MOTION_MANAGER startMotionCapture];
}

- (void) stopMotion
{
    LOGME
    [MOTION_MANAGER stopMotionCapture];
}

- (void) startVideoCapture
{
    LOGME
    
    [self hide2dTape];

    newMeasurement = [TMMeasurement getNewMeasurement];
    newMeasurement.type = self.type;
    [newMeasurement autoSelectUnitsScale];
    [self updateDistanceLabel];
    
    [VIDEO_MANAGER startVideoCapture];
    [VIDEO_MANAGER setDelegate:nil];
    [SENSOR_FUSION startSensorFusionWithDevice:[SESSION_MANAGER videoDevice]];
}

- (void)stopVideoCapture
{
    LOGME
    [VIDEO_MANAGER setDelegate:self.arView.videoView];
    [VIDEO_MANAGER stopVideoCapture];
    if([SENSOR_FUSION isSensorFusionRunning]) [SENSOR_FUSION stopSensorFusion];
}

- (void)startMeasuring
{
    LOGME
    [TMAnalytics
     startTimedEvent:@"Measurement.New"
     withParameters:@{ @"Type": [newMeasurement getTypeString] }
     ];
    [SENSOR_FUSION resetOrigin];
}

- (void)stopMeasuring
{
    LOGME
    [TMAnalytics endTimedEvent:@"Measurement.New"];
}

#pragma mark - RCSensorFusionDelegate

- (void) sensorFusionDidChangeStatus:(RCSensorFusionStatus *)status
{
    double currentTime = CACurrentMediaTime();
    if(status.errorCode == RCSensorFusionErrorCodeTooFast) {
        [self handleStateEvent:EV_FASTFAIL];
        if(currentState == ST_FASTFAIL) {
            lastFailTime = currentTime;
        }
        [SENSOR_FUSION startSensorFusionWithDevice:[SESSION_MANAGER videoDevice]];
    } else if(status.errorCode == RCSensorFusionErrorCodeOther) {
        [self handleStateEvent:EV_FAIL];
        if(currentState == ST_FAIL) {
            lastFailTime = currentTime;
        }
        [SENSOR_FUSION startSensorFusionWithDevice:[SESSION_MANAGER videoDevice]];
    } else if(status.errorCode == RCSensorFusionErrorCodeVision) {
        [self handleStateEvent:EV_VISIONFAIL];
        if(currentState == ST_VISIONFAIL) {
            lastFailTime = currentTime;
            [SENSOR_FUSION startSensorFusionWithDevice:[SESSION_MANAGER videoDevice]];
        }
    }
    if(lastFailTime == currentTime) {
        //in case we aren't changing states, update the error message
        NSString *message = [NSString stringWithFormat:@(setups[currentState].message), filterStatusCode];
        [self showMessage:message withTitle:@(setups[currentState].title) autoHide:setups[currentState].autohide];
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
        [self.arView.videoView displaySampleBuffer:data.sampleBuffer];
        [self.arView.featuresLayer updateFeatures:data.featurePoints];
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
    
    newMeasurement.rotationX = transformation.rotation.x;
    newMeasurement.rotationX_stdev = transformation.rotation.stdx;
    newMeasurement.rotationY = transformation.rotation.y;
    newMeasurement.rotationY_stdev = transformation.rotation.stdy;
    newMeasurement.rotationZ = transformation.rotation.z;
    newMeasurement.rotationZ_stdev = transformation.rotation.stdz;
    
    [newMeasurement autoSelectUnitsScale];
    
    [self updateDistanceLabel];
    [self.tapeView2D moveTapeWithXDisp:newMeasurement.xDisp withDistance:[newMeasurement getPrimaryMeasurementDist] withUnits:newMeasurement.units];
}

- (void)saveMeasurement
{
    LOGME
    newMeasurement.type = self.type;
    newMeasurement.timestamp = [[NSDate date] timeIntervalSince1970];
    newMeasurement.syncPending = YES;
    
    [newMeasurement insertIntoDb]; //order is important. this must be inserted before location is added.
    
    CLLocation *clLocation = [LOCATION_MANAGER getStoredLocation];
    TMLocation *locationObj;
    
    //add location to measurement
    if(useLocation && clLocation)
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
            
            if([LOCATION_MANAGER getStoredLocationAddress]) locationObj.address = [LOCATION_MANAGER getStoredLocationAddress];
            [locationObj insertIntoDb];
        }
        
        [locationObj addMeasurementObject:newMeasurement];
    }
    
    [DATA_MANAGER saveContext];
    
    NSNumber* primaryDist = [NSNumber numberWithFloat:[[newMeasurement getPrimaryDistanceObject] meters]];
    [TMAnalytics
     logEvent:@"Measurement.Save"
     withParameters:[NSDictionary dictionaryWithObjectsAndKeys: [newMeasurement getDistRangeString], @"Distance", nil]
     ];
    
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
    [self transitionToState:ST_READY];
}

- (void) saveAndGotoResult
{
    [self saveMeasurement];
    [self performSegueWithIdentifier:@"toResult" sender:self.btnRetry];
}

- (void) prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if ([[segue identifier] isEqualToString:@"toResult"])
    {
        TMResultsVC* resultsVC = [segue destinationViewController];
        resultsVC.theMeasurement = newMeasurement;
        resultsVC.prevView = self;
    }
    else if([[segue identifier] isEqualToString:@"toOptions"])
    {
        [self endAVSessionInBackground];
        
        TMOptionsVC *optionsVC = [segue destinationViewController];
        optionsVC.theMeasurement = newMeasurement;
        
        [[segue destinationViewController] setDelegate:self];
    }
}

- (void) endAVSessionInBackground
{
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        [SESSION_MANAGER endSession];
    });
}

@end
