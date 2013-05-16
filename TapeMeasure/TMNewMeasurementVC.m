//
//  TMViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMNewMeasurementVC.h"

@implementation TMNewMeasurementVC
{
    TMMeasurement *newMeasurement;
    
    BOOL useLocation;
    BOOL locationAuthorized;
    
    TMCrosshairsLayerDelegate *crosshairsDelegate;
    TMTargetLayerDelegate *targetDelegate;
    CALayer *targetLayer, *crosshairsLayer;
    TMFeaturesLayer* featuresLayer;
    CALayer* tickMarksLayer;
    TMTickMarksLayerDelegate* tickMarksDelegate;
    
    MBProgressHUD *progressView;
    
    NSMutableArray* pointsPool;
    struct corvis_feature_info features[FEATURE_COUNT];
    float videoScale;
    int videoFrameOffset;
    float screenWidthIn;
    float screenWidthCM;
    float pixelsPerInch;
    float pixelsPerCM;
    
    double lastTransitionTime;
    double lastFailTime;
    int filterFailCode;
    bool isAligned;
    bool isVisionWarning;
}

const double stateTimeout = 3.;
const double failTimeout = 2.;

typedef enum
{
    ICON_HIDDEN, ICON_RED, ICON_YELLOW, ICON_GREEN
} IconType;

enum state { ST_STARTUP, ST_FOCUS, ST_FIRSTFOCUS, ST_FIRSTCALIBRATION, ST_CALIB_ERROR, ST_INITIALIZING, ST_MOREDATA, ST_READY, ST_MEASURE, ST_MEASURE_STEADY, ST_ALIGN, ST_VISIONWARN, ST_FINISHED, ST_VISIONFAIL, ST_FASTFAIL, ST_FAIL, ST_SLOWDOWN, ST_ANY } currentState;
enum event { EV_RESUME, EV_FIRSTTIME, EV_CONVERGED, EV_STEADY_TIMEOUT, EV_VISIONFAIL, EV_FASTFAIL, EV_FAIL, EV_FAIL_EXPIRED, EV_SPEEDWARNING, EV_NOSPEEDWARNING, EV_TAP, EV_TAP_UNALIGNED, EV_TAP_WARNING, EV_ALIGN, EV_PAUSE, EV_CANCEL };

typedef struct { enum state state; enum event event; enum state newstate; } transition;

typedef struct
{
    enum state state;
    IconType icon;
    bool autofocus;
    bool datacapture;
    bool measuring;
    bool showDistance;
    bool features;
    bool progress;
    const char *title;
    const char *message;
    bool autohide;
} statesetup;

statesetup setups[] =
{
    { ST_STARTUP, ICON_YELLOW, true, false, false, false, false, false, "Initializing", "Move the device around very slowly and smoothly, while keeping some blue dots in sight.", false},
    { ST_FOCUS, ICON_YELLOW, true, false, false, false, false, false, "Focusing", "Point the camera at an area with lots of visual detail, and tap the screen to lock the focus.", false},
    { ST_FIRSTFOCUS, ICON_YELLOW, true, false, false, false, false, false, "Focusing", "We need to calibrate your device just once. Point the camera at something well-lit and visually complex, like a bookcase, and tap to lock the focus.", false},
    { ST_FIRSTCALIBRATION, ICON_YELLOW, false, true, false, false, true, true, "Calibrating", "Please move the device around very slowly to calibrate it. Slowly rotate the device from side to side as you go. Keep some dots in sight.", false},
    { ST_CALIB_ERROR, ICON_YELLOW, false, true, false, false, true, true, "Calibrating", "This might take a couple attempts. Be sure to move very slowly, and try rotating your device from side to side. Code %04x.", false},
    { ST_INITIALIZING, ICON_YELLOW, false, true, false, false, true, true, "Initializing", "Move the device around very slowly and smoothly, while keeping some blue dots in sight.", false},
    { ST_MOREDATA, ICON_YELLOW, false, true, false, false, true, true, "Initializing", "Move the device around very slowly and smoothly, while keeping some blue dots in sight.", false },
    { ST_READY, ICON_GREEN, false, true, false, true, false, false, "Ready", "Move the device to one end of the thing you want to measure, and tap the screen to start.", false },
    { ST_MEASURE, ICON_GREEN, false, true, true, true, true, false, "Measuring", "Move the device to the other end of what you're measuring. We'll show you how far it moved.", false },
    { ST_MEASURE_STEADY, ICON_GREEN, false, true, true, true, true, false, "Measuring", "Tap the screen to finish.", false },
    { ST_ALIGN, ICON_GREEN, true, false, false, true, false, false, "Finished", "Looks good. Press save to name and store your measurement.", false },
    { ST_VISIONWARN, ICON_YELLOW, true, false, false, true, false, false, "Finished", "It was hard to see at times during the measurement, so it might be inaccurate. You can still save it.", false },
    { ST_FINISHED, ICON_GREEN, true, false, false, true, false, false, "Finished", "Looks good. Press save to name and store your measurement.", false },
    { ST_VISIONFAIL, ICON_RED, false, true, false, true, true, false, "Try again", "Sorry, I can't see well enough to measure right now. Try to keep some blue dots in sight, and make sure the area is well lit. Error code %04x.", false },
    { ST_FASTFAIL, ICON_RED, false, true, false, true, true, false, "Try again", "Sorry, that didn't work. Try to move very slowly and smoothly to get accurate measurements. Error code %04x.", false },
    { ST_FAIL, ICON_RED, false, true, false, true, true, false, "Try again", "Sorry, we need to try that again. If that doesn't work send error code %04x to support@realitycap.com.", false },
    { ST_SLOWDOWN, ICON_YELLOW, false, true, true, true, true, false, "Measuring", "Slow down please. You'll get the most accurate measurements by moving very slowly and smoothly.", false }
};

transition transitions[] =
{
    { ST_STARTUP, EV_RESUME, ST_FOCUS },
    { ST_STARTUP, EV_FIRSTTIME, ST_FIRSTFOCUS },
    { ST_FIRSTFOCUS, EV_TAP, ST_FIRSTCALIBRATION },
    { ST_FOCUS, EV_TAP, ST_INITIALIZING },
    { ST_FIRSTCALIBRATION, EV_CONVERGED, ST_READY },
    { ST_FIRSTCALIBRATION, EV_STEADY_TIMEOUT, ST_CALIB_ERROR },
    { ST_CALIB_ERROR, EV_CONVERGED, ST_READY },
    { ST_INITIALIZING, EV_CONVERGED, ST_READY },
    { ST_INITIALIZING, EV_STEADY_TIMEOUT, ST_MOREDATA },
    { ST_MOREDATA, EV_CONVERGED, ST_READY },
    { ST_MOREDATA, EV_VISIONFAIL, ST_VISIONFAIL },
    { ST_MOREDATA, EV_FASTFAIL, ST_FASTFAIL },
    { ST_MOREDATA, EV_FAIL, ST_FAIL },
    { ST_READY, EV_TAP, ST_MEASURE },
    { ST_READY, EV_VISIONFAIL, ST_VISIONFAIL },
    { ST_READY, EV_FASTFAIL, ST_FASTFAIL },
    { ST_READY, EV_FAIL, ST_FAIL },
    { ST_MEASURE, EV_STEADY_TIMEOUT, ST_MEASURE_STEADY },
    { ST_MEASURE, EV_SPEEDWARNING, ST_SLOWDOWN },
    { ST_MEASURE, EV_VISIONFAIL, ST_VISIONFAIL },
    { ST_MEASURE, EV_FASTFAIL, ST_FASTFAIL },
    { ST_MEASURE, EV_FAIL, ST_FAIL },
    { ST_MEASURE_STEADY, EV_TAP, ST_FINISHED },
    { ST_MEASURE_STEADY, EV_TAP_UNALIGNED, ST_ALIGN },
    { ST_MEASURE_STEADY, EV_TAP_WARNING, ST_VISIONWARN },
    { ST_MEASURE_STEADY, EV_FASTFAIL, ST_FASTFAIL },
    { ST_MEASURE_STEADY, EV_FAIL, ST_FAIL },
    { ST_ALIGN, EV_PAUSE, ST_ALIGN },
    { ST_VISIONWARN, EV_PAUSE, ST_VISIONWARN },
    { ST_FINISHED, EV_PAUSE, ST_FINISHED },
    { ST_VISIONFAIL, EV_FAIL_EXPIRED, ST_INITIALIZING },
    { ST_FASTFAIL, EV_FAIL_EXPIRED, ST_INITIALIZING },
    { ST_FAIL, EV_FAIL_EXPIRED, ST_INITIALIZING },
    { ST_SLOWDOWN, EV_TAP, ST_FINISHED },
    { ST_SLOWDOWN, EV_NOSPEEDWARNING, ST_MEASURE },
    { ST_SLOWDOWN, EV_VISIONFAIL, ST_VISIONFAIL },
    { ST_SLOWDOWN, EV_FASTFAIL, ST_FASTFAIL },
    { ST_SLOWDOWN, EV_FAIL, ST_FAIL },
    { ST_ANY, EV_PAUSE, ST_STARTUP },
    { ST_ANY, EV_CANCEL, ST_STARTUP }
};

#define TRANS_COUNT (sizeof(transitions) / sizeof(*transitions))

- (void) transitionToState:(int)newState
{
    if(currentState == newState) return;
    statesetup oldSetup = setups[currentState];
    statesetup newSetup = setups[newState];
    
    if(oldSetup.autofocus && !newSetup.autofocus)
        [SESSION_MANAGER lockFocus];
    if(!oldSetup.autofocus && newSetup.autofocus)
        [SESSION_MANAGER unlockFocus];
    if(oldSetup.measuring && !newSetup.measuring)
        [self stopMeasuring];
    if(!oldSetup.datacapture && newSetup.datacapture)
        [self startDataCapture];
    if(!oldSetup.measuring && newSetup.measuring)
        [self startMeasuring];
    if(oldSetup.measuring && !newSetup.measuring)
        [self stopMeasuring];
    if(oldSetup.datacapture && !newSetup.datacapture)
        [self shutdownDataCapture];
    if(!oldSetup.showDistance && newSetup.showDistance)
        [self showDistanceLabel];
    if(oldSetup.showDistance && !newSetup.showDistance)
        [self hideDistanceLabel];
    if(oldSetup.features && !newSetup.features)
        [self hideFeatures];
    if(!oldSetup.features && newSetup.features)
        [self showFeatures];
    if(oldSetup.progress && !newSetup.progress)
        [self hideProgress];
    if(!oldSetup.progress && newSetup.progress)
        [self showProgressWithTitle:[NSString stringWithCString:newSetup.title encoding:NSASCIIStringEncoding]];
    currentState = newState;

    [self showIcon:newSetup.icon];

    NSString *message = [NSString stringWithFormat:[NSString stringWithCString:newSetup.message encoding:NSASCIIStringEncoding], filterFailCode];
    [self showMessage:message withTitle:[NSString stringWithCString:newSetup.title encoding:NSASCIIStringEncoding] autoHide:newSetup.autohide];

    lastTransitionTime = CACurrentMediaTime();
}

- (void)handleStateEvent:(int)event
{
    int newState = currentState;
    for(int i = 0; i < TRANS_COUNT; ++i) {
        if(transitions[i].state == currentState || transitions[i].state == ST_ANY) {
            if(transitions[i].event == event) {
                newState = transitions[i].newstate;
                NSLog(@"State transition from %d to %d", currentState, newState);
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
    
    useLocation = [LOCATION_MANAGER isLocationAuthorized] && [[NSUserDefaults standardUserDefaults] boolForKey:PREF_ADD_LOCATION];
    
    [self setupVideoPreview];
    
    //setup screen tap detection
    UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapGesture:)];
    tapGesture.numberOfTapsRequired = 1;
    [self.videoPreviewView addGestureRecognizer:tapGesture];
        
    [self setupFeatureDisplay];
    
    screenWidthCM = [RCDeviceInfo getPhysicalScreenMetersX] * 100;
    pixelsPerCM = self.distanceBg.frame.size.width / screenWidthCM;
    screenWidthIn = [RCDeviceInfo getPhysicalScreenMetersX] * INCHES_PER_METER;
    pixelsPerInch = self.distanceBg.frame.size.width / screenWidthIn;
}

- (void)viewDidUnload
{
	LOGME
	[self setLblDistance:nil];
	[self setLblInstructions:nil];	[self setVideoPreviewView:nil];
//    [self setBtnPageCurl:nil];
    [self setInstructionsBg:nil];
    [self setDistanceBg:nil];
    [self setBtnSave:nil];
//    [self setLocationButton:nil];
    [self setStatusIcon:nil];
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
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleAVSessionError)
                                                 name:AVCaptureSessionRuntimeErrorNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleAVSessionInterrupted)
                                                 name:AVCaptureSessionInterruptionEndedNotification
                                               object:nil];
    [self handleResume];
}

- (void) viewWillDisappear:(BOOL)animated
{
    LOGME
    [self handleStateEvent:EV_CANCEL];
    [super viewWillDisappear:animated];
    [self.navigationController setToolbarHidden:NO animated:animated];
    self.navigationController.navigationBar.translucent = NO;
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [self endAVSessionInBackground];
}

- (void) willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
    [UIView setAnimationsEnabled:NO]; //disable weird rotation animation on video preview
    [super willRotateToInterfaceOrientation:toInterfaceOrientation duration:duration];
}

- (void) didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
    [super didRotateFromInterfaceOrientation:fromInterfaceOrientation];
    [self setupVideoPreviewFrame];
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
	//watch inertial sensors on background thread
//	[self performSelectorInBackground:(@selector(watchDeviceMotion)) withObject:nil];
    if (![SESSION_MANAGER isRunning]) [SESSION_MANAGER startSession]; //might not be running due to app pause
    if([RCCalibration hasCalibrationData]) {
        [self handleStateEvent:EV_RESUME];
    } else {
        [self handleStateEvent:EV_FIRSTTIME];
    }
    
    [self setupTickMarksLayer];
}

//the follow two methods are temp, for testing
- (void)handleAVSessionError
{
    NSLog(@"AV session error");
    [self showMessage:@"AV session error" withTitle:@"Error" autoHide:NO];
}

- (void)handleAVSessionInterrupted
{
    NSLog(@"AV session interrupted");
    [self showMessage:@"AV session interrupted" withTitle:@"Error" autoHide:NO];
}

//handles button tap event
- (IBAction)handleButtonTap:(id)sender
{
    //Not currently used
}

-(void) handleTapGesture:(UIGestureRecognizer *) sender {
    if (sender.state != UIGestureRecognizerStateEnded) return;
    if(isVisionWarning)
        [self handleStateEvent:EV_TAP_WARNING];
    if(!isAligned)
        [self handleStateEvent:EV_TAP_UNALIGNED];
    [self handleStateEvent:EV_TAP]; //always send the tap - align ignores it and others might need it
}

- (void)setupVideoPreview
{
    LOGME

    self.videoPreviewView.clipsToBounds = YES;
    SESSION_MANAGER.videoPreviewLayer.videoGravity = AVLayerVideoGravityResizeAspectFill; //fill view, cropping if necessary
    
    [self setupVideoPreviewFrame];
    [self.videoPreviewView.layer addSublayer:SESSION_MANAGER.videoPreviewLayer];
    
    float circleRadius = 40.;
    crosshairsDelegate = [[TMCrosshairsLayerDelegate alloc] initWithRadius:circleRadius];
    crosshairsLayer = [CALayer new];
    [crosshairsLayer setDelegate:crosshairsDelegate];
    crosshairsLayer.hidden = YES;
    crosshairsLayer.frame = self.videoPreviewView.frame;
    [crosshairsLayer setNeedsDisplay];
    [self.videoPreviewView.layer addSublayer:crosshairsLayer];
    
    targetDelegate = [[TMTargetLayerDelegate alloc] initWithRadius:circleRadius];
    targetLayer = [CALayer new];
    [targetLayer setDelegate:targetDelegate];
    targetLayer.hidden = YES;
    targetLayer.frame = CGRectMake(self.videoPreviewView.frame.size.width / 2 - circleRadius, self.videoPreviewView.frame.size.height / 2 - circleRadius, circleRadius * 2, circleRadius * 2);
    [targetLayer setNeedsDisplay];
    [self.videoPreviewView.layer insertSublayer:targetLayer below:crosshairsLayer];
    
    featuresLayer = [[TMFeaturesLayer alloc] initWithFeatureCount:FEATURE_COUNT];
    featuresLayer.hidden = YES;
    featuresLayer.frame = self.videoPreviewView.frame;
    [featuresLayer setNeedsDisplay];
    [self.videoPreviewView.layer insertSublayer:featuresLayer below:targetLayer];
}

- (void) setupVideoPreviewFrame
{
    LOGME

    if ([SESSION_MANAGER.videoPreviewLayer respondsToSelector:@selector(connection)])
    {
        if ([SESSION_MANAGER.videoPreviewLayer.connection isVideoOrientationSupported])
        {
            [SESSION_MANAGER.videoPreviewLayer.connection setVideoOrientation:UIPrintInfoOrientationLandscape];
        }
    }
    else
    {
        // Deprecated in 6.0; here for backward compatibility
        if ([SESSION_MANAGER.videoPreviewLayer isOrientationSupported])
        {
            [SESSION_MANAGER.videoPreviewLayer setOrientation:UIPrintInfoOrientationLandscape];
        }
    }
    
    CGRect videoRect = self.videoPreviewView.bounds;
    SESSION_MANAGER.videoPreviewLayer.frame = videoRect;
}

- (void) setupFeatureDisplay
{
    // create a pool of point objects to use in feature display
    pointsPool = [[NSMutableArray alloc] initWithCapacity:FEATURE_COUNT];
    for (int i = 0; i < FEATURE_COUNT; i++)
    {
        TMPoint* point = (TMPoint*)[DATA_MANAGER getNewObjectOfType:[TMPoint getEntity]];
        [pointsPool addObject:point];
    }
    
    // create the array of feature structs that we pass into corvis
    for (int i = 0; i < FEATURE_COUNT; i++)
    {
        struct corvis_feature_info newFeature;
        features[i] = newFeature;
    }
    
//    //for testing
//    pointsPool = [[NSMutableArray alloc] initWithCapacity:FEATURE_COUNT];
//    for (int i = 0; i < FEATURE_COUNT; i++)
//    {
//        TMPoint* point = (TMPoint*)[DATA_MANAGER getNewObjectOfType:[TMPoint getEntity]];
//        point.imageX = arc4random_uniform(featuresLayer.frame.size.width);
//        point.imageY = arc4random_uniform(featuresLayer.frame.size.width);
//        point.quality = 1.;
//        [pointsPool addObject:point];
//    }
    
    // the scale of the video vs the video preview frame
    videoScale = (float)self.videoPreviewView.frame.size.width / (float)VIDEO_WIDTH;
    
    // videoFrameOffset is necessary to align the features properly. the video is being cropped to fit the view, which is slightly less tall than the video
    videoFrameOffset = (lrintf(VIDEO_HEIGHT * videoScale) - self.videoPreviewView.frame.size.height) / 2;
}

- (void)setupTickMarksLayer
{    
    [[NSUserDefaults standardUserDefaults] synchronize]; //in case user just changed default setting
    
    if (tickMarksLayer == nil)
    {
        tickMarksLayer = [CALayer new];
    }
    else
    {
        tickMarksLayer.delegate = nil;
        tickMarksDelegate = nil;
    }
    
    tickMarksDelegate = [[TMTickMarksLayerDelegate alloc] initWithWidthMeters:[RCDeviceInfo getPhysicalScreenMetersX] withUnits:(Units)[[NSUserDefaults standardUserDefaults] integerForKey:PREF_UNITS]];
    [tickMarksLayer setDelegate:tickMarksDelegate];
    tickMarksLayer.hidden = YES;
    tickMarksLayer.frame = CGRectMake(self.view.frame.origin.x, self.view.frame.origin.y, self.view.frame.size.width * 2, self.view.frame.size.height);
    [tickMarksLayer setNeedsDisplay];
    [self.distanceBg.layer addSublayer:tickMarksLayer];    
}

- (void)startDataCapture
{
    LOGME
    
    [self hideDistanceLabel];
    
    //make sure we have up to date location data
    if (useLocation) [LOCATION_MANAGER startLocationUpdates];
    
    newMeasurement = [TMMeasurement getNewMeasurement];
    newMeasurement.type = self.type;
    [newMeasurement autoSelectUnitsScale];
    self.lblDistance.text = [newMeasurement getFormattedDistance:0.0];

    CLLocation *loc = [LOCATION_MANAGER getStoredLocation];
    
    __weak TMNewMeasurementVC* weakSelf = self;
    [CORVIS_MANAGER
     setupPluginsWithFilter:true
     withCapture:false
     withReplay:false
     withLocationValid:loc ? true : false
     withLatitude:loc ? loc.coordinate.latitude : 0
     withLongitude:loc ? loc.coordinate.longitude : 0
     withAltitude:loc ? loc.altitude : 0
     withStatusCallback:^(bool measurement_active, float x, float stdx, float y, float stdy, float z, float stdz, float path, float stdpath, float rx, float stdrx, float ry, float stdry, float rz, float stdrz, float orientx, float orienty, int code, float converged, bool steady, bool aligned, bool speed_warning, bool vision_warning, bool vision_failure, bool speed_failure, bool other_failure) {

         filterFailCode = code;
         double currentTime = CACurrentMediaTime();
         if(speed_failure) {
             [weakSelf handleStateEvent:EV_FASTFAIL];
             lastFailTime = currentTime;
         } else if(other_failure) {
             [weakSelf handleStateEvent:EV_FAIL];
             lastFailTime = currentTime;
         } else if(vision_failure) {
             [weakSelf handleStateEvent:EV_VISIONFAIL];
             lastFailTime = currentTime;
         }
         if(lastFailTime == currentTime) {
             //in case we aren't changing states, update the error message
             NSString *message = [NSString stringWithFormat:[NSString stringWithCString:setups[currentState].message encoding:NSASCIIStringEncoding], filterFailCode];
             [weakSelf showMessage:message withTitle:[NSString stringWithCString:setups[currentState].title encoding:NSASCIIStringEncoding] autoHide:setups[currentState].autohide];
         }
         double time_in_state = currentTime - lastTransitionTime;
         [weakSelf updateProgress:converged];
         if(converged >= 1.) {
             if(currentState == ST_FIRSTCALIBRATION || currentState == ST_CALIB_ERROR) {
                 [CORVIS_MANAGER stopMeasurement]; //get corvis to store the parameters
                 [CORVIS_MANAGER saveDeviceParameters];
             }
             [weakSelf handleStateEvent:EV_CONVERGED];
         }
         if(steady && time_in_state > stateTimeout) [self handleStateEvent:EV_STEADY_TIMEOUT];
         
         double time_since_fail = currentTime - lastFailTime;
         if(time_since_fail > failTimeout) [weakSelf handleStateEvent:EV_FAIL_EXPIRED];
         
         if(speed_warning) [weakSelf handleStateEvent:EV_SPEEDWARNING];
         else [weakSelf handleStateEvent:EV_NOSPEEDWARNING];
    
         if(aligned) [weakSelf handleStateEvent:EV_ALIGN];
         isAligned = aligned;
         
         isVisionWarning = vision_warning;
         
         [weakSelf updateOverlayWithX:orientx withY:orienty];
         
         if(measurement_active) [weakSelf updateMeasurementDataWithX:x
                                                            stdx:stdx
                                                               y:y
                                                            stdy:stdy
                                                               z:z
                                                            stdz:stdz
                                                            path:path
                                                         stdpath:stdpath
                                                              rx:rx
                                                           stdrx:stdrx
                                                              ry:ry
                                                           stdry:stdry
                                                              rz:rz
                                                           stdrz:stdrz];

     }];
    
    [CORVIS_MANAGER startPlugins];
    [MOTIONCAP_MANAGER startMotionCap];
    [VIDEOCAP_MANAGER startVideoCap];
}

- (void)startMeasuring
{
    LOGME
    
    [TMAnalytics
     logEvent:@"Measurement.Start"
     withParameters:[NSDictionary dictionaryWithObjectsAndKeys:useLocation ? @"Yes" : @"No", @"WithLocation", nil]
     ];
    
    [self updateMeasurementDataWithX:0 stdx:0 y:0 stdy:0 z:0 stdz:0 path:0 stdpath:0 rx:0 stdrx:0 ry:0 stdry:0 rz:0 stdrz:0];
    
    self.btnSave.enabled = NO;

    [CORVIS_MANAGER startMeasurement];
}

- (void)updateMeasurementDataWithX:(float)x stdx:(float)stdx y:(float)y stdy:(float)stdy z:(float)z stdz:(float)stdz path:(float)path stdpath:(float)stdpath rx:(float)rx stdrx:(float)stdrx ry:(float)ry stdry:(float)stdry rz:(float)rz stdrz:(float)stdrz
{
    newMeasurement.xDisp = x;
    newMeasurement.xDisp_stdev = stdx;
    newMeasurement.yDisp = y;
    newMeasurement.yDisp_stdev = stdy;
    newMeasurement.zDisp = z;
    newMeasurement.zDisp_stdev = stdz;
    newMeasurement.totalPath = path;
    newMeasurement.totalPath_stdev = stdpath;
    float ptdist = sqrt(x*x + y*y + z*z);
    newMeasurement.pointToPoint = ptdist;
    float hdist = sqrt(x*x + y*y);
    newMeasurement.horzDist = hdist;
    float hxlin = x / hdist * stdx, hylin = y / hdist * stdy;
    newMeasurement.horzDist_stdev = sqrt(hxlin * hxlin + hylin * hylin);
    float ptxlin = x / ptdist * stdx, ptylin = y / ptdist * stdy, ptzlin = z / ptdist * stdz;
    newMeasurement.pointToPoint_stdev = sqrt(ptxlin * ptxlin + ptylin * ptylin + ptzlin * ptzlin);
    newMeasurement.rotationX = rx;
    newMeasurement.rotationX_stdev = stdrx;
    newMeasurement.rotationY = ry;
    newMeasurement.rotationY_stdev = stdry;
    newMeasurement.rotationZ = rz;
    newMeasurement.rotationZ_stdev = stdrz;
    
    [newMeasurement autoSelectUnitsScale];
    self.lblDistance.text = [newMeasurement getFormattedDistance:[newMeasurement getPrimaryMeasurementDist]];
    
    [self moveTapeWithXDisp:x];
}

- (void)stopMeasuring
{
    LOGME
    
    [CORVIS_MANAGER stopMeasurement];
    [CORVIS_MANAGER saveDeviceParameters];

    [TMAnalytics logEvent:@"Measurement.Stop"];
    
    self.btnSave.enabled = YES;
}

- (void)shutdownDataCapture
{
    LOGME
    
    [VIDEOCAP_MANAGER stopVideoCap];
    [MOTIONCAP_MANAGER stopMotionCap];
    
    [NSThread sleepForTimeInterval:0.2]; //hack to prevent CorvisManager from receiving a video frame after plugins have stopped.
    
    [CORVIS_MANAGER stopPlugins];
    [CORVIS_MANAGER teardownPlugins];
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
    
    [TMAnalytics logEvent:@"Measurement.Save"];
    
    if (locationObj.syncPending)
    {
        __weak TMNewMeasurementVC* weakSelf = self;
        [locationObj
         postToServer:^(int transId)
         {
             NSLog(@"Post location success callback");
             locationObj.syncPending = NO;
             [DATA_MANAGER saveContext];
             [weakSelf postMeasurement];
         }
         onFailure:^(int statusCode)
         {
             NSLog(@"Post location failure callback");
         }
         ];
    }
    else
    {
        [self postMeasurement];
    }

    [self postCalibrationToServer];
}

- (void)postCalibrationToServer
{
    LOGME
        
    [SERVER_OPS
     postDeviceCalibration:^{
         NSLog(@"postCalibrationToServer success");
     }
     onFailure:^(int statusCode) {
         NSLog(@"postCalibrationToServer failed with status code %i", statusCode);
     }
     ];
}

- (void)showFeatures
{
    featuresLayer.hidden = NO;
}

- (void)hideFeatures
{
    featuresLayer.hidden = YES;
}

- (void)showTickMarks
{
    tickMarksLayer.hidden = NO;
    [tickMarksLayer needsLayout];
}

- (void)hideTickMarks
{
    tickMarksLayer.hidden = YES;
    [tickMarksLayer needsLayout];
}

- (void)updateOverlayWithX:(float)x withY:(float)y
{
    float centerX = self.videoPreviewView.frame.size.width / 2 - (y * self.videoPreviewView.frame.size.width);
    float centerY = self.videoPreviewView.frame.size.height / 2 + (x * self.videoPreviewView.frame.size.width);

    //constrain target location to bounds of frame
    centerX = centerX > self.videoPreviewView.frame.size.width ? self.videoPreviewView.frame.size.width : centerX;
    centerX = centerX < 0 ? 0 : centerX;
    centerY = centerY > self.videoPreviewView.frame.size.height ? self.videoPreviewView.frame.size.height : centerY;
    centerY = centerY < 0 ? 0 : centerY;

    float radius = targetLayer.frame.size.height / 2;
    targetLayer.frame = CGRectMake(centerX - radius, centerY - radius, radius * 2, radius * 2);
    if(!targetLayer.hidden) [targetLayer needsLayout];
    
    int count = [CORVIS_MANAGER getCurrentFeatures:features withMax:FEATURE_COUNT];
    NSMutableArray* trackedFeatures = [NSMutableArray arrayWithCapacity:count]; // the points we will display on screen
    for (int i = 0; i < count; i++)
    {
        TMPoint* point = [pointsPool objectAtIndex:i]; //get a point from the pool
        point.imageX = self.videoPreviewView.frame.size.width - lrintf(features[i].y * videoScale);
        point.imageY = lrintf(features[i].x * videoScale) - videoFrameOffset;
        point.quality = features[i].quality;
        [trackedFeatures addObject:point];
    }
    
    [featuresLayer setFeaturePositions:trackedFeatures];
//    [featuresLayer setFeaturePositions:pointsPool]; //for testing
}

- (void)moveTapeWithXDisp:(float)x
{
    float xOffset = 0;
    
    if (newMeasurement.units == UnitsImperial)
    {
        float inches = [newMeasurement getPrimaryMeasurementDist] * INCHES_PER_METER;
        float distRemainder = inches - floor(inches);
        xOffset = distRemainder * pixelsPerInch;
        
        if (x > 0) xOffset = -xOffset;
        xOffset = xOffset - pixelsPerInch;
    }
    else
    {
        float centimeters = [newMeasurement getPrimaryMeasurementDist] * 100;
        float distRemainder = centimeters - floor(centimeters);
        xOffset = distRemainder * pixelsPerCM;
        
        if (x > 0) xOffset = -xOffset;
        xOffset = xOffset - pixelsPerCM;
    }
    
    tickMarksLayer.frame = CGRectMake(xOffset, tickMarksLayer.frame.origin.y, tickMarksLayer.frame.size.width, tickMarksLayer.frame.size.height);
    [tickMarksLayer needsLayout];
}

- (void)showProgressWithTitle:(NSString*)title
{
    progressView = [[MBProgressHUD alloc] initWithView:self.navigationController.view];
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

-(void)postMeasurement
{
    [newMeasurement
     postToServer:
     ^(int transId)
     {
         NSLog(@"postMeasurement success callback");
         newMeasurement.syncPending = NO;
         [DATA_MANAGER saveContext];
     }
     onFailure:
     ^(int statusCode)
     {
         //TODO: handle error
         NSLog(@"Post measurement failure callback");
     }
     ];
}

- (void)showIcon:(IconType)type
{
    switch (type) {
        case ICON_HIDDEN:
            self.statusIcon.hidden = YES;
            break;

        case ICON_GREEN:
            self.statusIcon.image = [UIImage imageNamed:@"go_small"];
            self.statusIcon.hidden = NO;
            break;
            
        case ICON_YELLOW:
            self.statusIcon.image = [UIImage imageNamed:@"caution_small"];
            self.statusIcon.hidden = NO;
            break;
            
        case ICON_RED:
            self.statusIcon.image = [UIImage imageNamed:@"stop_small"];
            self.statusIcon.hidden = NO;
            break;
            
        default:
            break;
    }
}

- (void)hideIcon
{
    self.statusIcon.hidden = YES;
}

- (void)showMessage:(NSString*)message withTitle:(NSString*)title autoHide:(BOOL)hide
{
    self.instructionsBg.hidden = NO;
    self.lblInstructions.hidden = NO;
    
    self.lblInstructions.text = message ? message : @"";    
    self.navigationController.navigationBar.topItem.title = title ? title : @"";
    
    self.instructionsBg.alpha = 0.3;
    self.lblInstructions.alpha = 1;
    
    if (hide)
    {
        int const delayTime = 5;
        int const fadeTime = 2;
        
        [self fadeOut:self.lblInstructions withDuration:fadeTime andWait:delayTime];
        [self fadeOut:self.instructionsBg withDuration:fadeTime andWait:delayTime];
    }
}

- (void)hideMessage
{
    [self fadeOut:self.lblInstructions withDuration:0.5 andWait:0];
    [self fadeOut:self.instructionsBg withDuration:0.5 andWait:0];
    
    self.navigationController.navigationBar.topItem.title = @"";
}

- (void)showDistanceLabel
{
    self.distanceBg.hidden = NO;
    self.lblDistance.hidden = NO;
    [self showTickMarks];
}

- (void)hideDistanceLabel
{
    self.distanceBg.hidden = YES;
    self.lblDistance.hidden = YES;
    [self hideTickMarks];
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

- (void)updateDistanceLabel
{
    NSString *distString = [newMeasurement getFormattedDistance:newMeasurement.pointToPoint];
	self.lblDistance.text = [NSString stringWithFormat:@"Distance: %@", distString];
}

//this routine is run in a background thread
//- (void) watchDeviceMotion
//{
//	LOGME
//	
//	//create log file and write column names as first line
//	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask, YES);
//    NSString *documentsDirectory = [paths objectAtIndex:0];
//    NSString *logFilePath = [documentsDirectory stringByAppendingPathComponent:@"log.txt"];
//    
//	NSString *colNames = @"timestamp,sensor,x,y,z\n";
//	bool isFileCreated = [colNames writeToFile:logFilePath atomically:YES encoding:NSUTF8StringEncoding error:nil];
//	if(!isFileCreated) NSLog(@"Failed to create log file");
//	
//    NSFileHandle *myHandle = [NSFileHandle fileHandleForWritingAtPath:logFilePath];
//	
//	NSString *logLine;
//    		
//	while (motionMan.isAccelerometerActive)  //we stop accelerometer updates when the app is paused or terminated, which will stop this thread.
//	{
//		//detect bump
//		float absAccel = fabs(motionMan.accelerometerData.acceleration.y);
//		if(!lastAccel) lastAccel = absAccel; //if lastAccel has not been set, make it equal to current accel
//		float accelChange = absAccel - lastAccel;
//		lastAccel = absAccel;
//		
//		if(accelChange > 0.3f) { //change this value to adjust bump sensitivity
//			[self performSelectorOnMainThread:(@selector(handleBump)) withObject:nil waitUntilDone:YES];
//		}
//		
//		//log inertial data
//		if(isMeasuring)
//		{
//			//append line to log
//			logLine = [NSString stringWithFormat:@"%f,accel,%f,%f,%f\n", motionMan.accelerometerData.timestamp, motionMan.accelerometerData.acceleration.x, motionMan.accelerometerData.acceleration.y, motionMan.accelerometerData.acceleration.z];
//		
//			[myHandle seekToEndOfFile];
//			[myHandle writeData:[logLine dataUsingEncoding:NSUTF8StringEncoding]];
//			
//			//append line to log
//			logLine = [NSString stringWithFormat:@"%f,gyro,%f,%f,%f\n", motionMan.gyroData.timestamp, motionMan.gyroData.rotationRate.x, motionMan.gyroData.rotationRate.y, motionMan.gyroData.rotationRate.z];
//			
//			[myHandle seekToEndOfFile];
//			[myHandle writeData:[logLine dataUsingEncoding:NSUTF8StringEncoding]];
//		}
//		
//		[NSThread sleepForTimeInterval: 1.0/60];
//	}
//	
//	[myHandle closeFile];
//}
//
//- (void) handleBump
//{
//	int currTime = CFAbsoluteTimeGetCurrent(); //time in sec
//	int timeElapsed = currTime - lastBump; //since last bump
//	
//	if(timeElapsed >= 1) { //allow another bump 1s after last bump
//		NSLog(@"Bump");
//		lastBump = currTime;
//		
//	}
//}
//
//- (IBAction)handlePageCurl:(id)sender
//{
//    [TMAnalytics logEvent:@"Measurement.ViewOptions.NewMeasurement"];
//}

- (IBAction)handleSaveButton:(id)sender
{
    [self saveMeasurement];
    [self performSegueWithIdentifier:@"toResult" sender:self.btnSave];
}

//- (IBAction)handleLocationButton:(id)sender {
//    LOGME
//    
//    useLocation = !useLocation;
//    
//    [self setLocationButtonState];
//}

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

- (void)didDismissOptions
{
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        [SESSION_MANAGER startSession];
    });
    [self updateDistanceLabel];
}

//- (void)setLocationButtonState
//{
//    if(useLocation)
//    {
//        self.locationButton.image = [UIImage imageNamed:@"ComposeSheetLocationArrowActive.png"];
//    }
//    else
//    {
//        self.locationButton.image = [UIImage imageNamed:@"ComposeSheetLocationArrow.png"];
//        if (!locationAuthorized) self.locationButton.enabled = NO;
//    }
//}

@end
