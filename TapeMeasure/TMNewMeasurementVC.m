//
//  TMViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMNewMeasurementVC.h"

@implementation TMNewMeasurementVC

typedef enum
{
    ICON_RED, ICON_YELLOW, ICON_GREEN
} IconType;

enum state { ST_STARTUP, ST_INITIALIZING, ST_MOREDATA, ST_READY, ST_MEASURE, ST_ALIGN, ST_FINISHED, ST_VISIONFAIL, ST_FASTFAIL, ST_FAIL, ST_SLOWDOWN, ST_ANY } currentState;

double lastTransitionTime;
double lastFailTime;

enum event { EV_RESUME, EV_CONVERGED, EV_CONVERGE_TIMEOUT, EV_FAIL_TIMEOUT, EV_VISIONFAIL, EV_FASTFAIL, EV_FAIL, EV_FAIL_EXPIRED, EV_SPEEDWARNING, EV_TAP, EV_ALIGN, EV_PAUSE, EV_CANCEL };

typedef struct { enum state state; enum event event; enum state newstate; } transition;

typedef struct
{
    enum state state;
    IconType icon;
    bool plugins;
    bool measuring;
    bool crosshairs;
    bool target;
    const char *title;
    const char *message;
    bool autohide;
} statesetup;

statesetup setups[] =
{
    { ST_STARTUP, ICON_YELLOW, false, false, false, false, "", "", true},
    { ST_INITIALIZING, ICON_YELLOW, true, false, false, false, "Initializing...", "Please hold your device parallel to what you want to measure, move to the starting point, and hold it steady.", false},
    { ST_MOREDATA, ICON_YELLOW, true, false, false, false, "Initializing...", "Sorry, I need a little more data before we can measure. Try gently moving your device around, then come back to your measurement start position.", false },
    { ST_READY, ICON_GREEN, true, false, true, false, "Ready to measure...",  "Please hold your device parallel to what you want to measure, move to the starting point, center one end of your measurement in the crosshairs, hold it steady, and tap to begin measuring.", false },
    { ST_MEASURE, ICON_GREEN, true, true, true, true, "Measuring...", "Keep the camera pointed in the same direction by keeping the target lined up in the middle of crosshairs, and slowly walk until the ending point of your measurement is aligned in the crosshairs. Tap to finish.", false },
    { ST_ALIGN, ICON_YELLOW, true, true, true, true, "Finishing...", "Before we finish the measurement, please line up the target and the ending point of your measurement with the middle of crosshairs. You'll need to move and turn to get everything aligned and get an accurate measurement.", false },
    { ST_FINISHED, ICON_GREEN, false, false, false, false, "Measurement complete.", "Looks good. Hit save to name and store your measurement.", true },
    { ST_VISIONFAIL, ICON_RED, true, false, false, false, "Try again...", "Sorry, I can't see well enough to measure right now. Are the lights on? If what you want to measure is not very visually distinctive (like a blank wall) you'll need to point the camera in a different direction. Error code %04x.", false },
    { ST_FASTFAIL, ICON_RED, true, false, false, false, "Try again...", "Sorry, we need to try that again. Make sure you move slowly and gently to get accurate measurements. If you're holding your device steady and you keep getting this message, please send error code %04x to support@realitycap.com.", false },
    { ST_FAIL, ICON_RED, true, false, false, false, "Try again...", "Sorry, we need to try that again. If you keep getting this message, please send error code %04x to support@realitycap.com.", false },
    { ST_SLOWDOWN, ICON_YELLOW, true, true, true, true, "Measuring", "Slow down. You'll get the most accurate measurements by moving slowly and smoothly.", false }
};

transition transitions[] =
{
    { ST_STARTUP, EV_RESUME, ST_INITIALIZING },
    { ST_INITIALIZING, EV_CONVERGED, ST_READY },
    { ST_INITIALIZING, EV_CONVERGE_TIMEOUT, ST_MOREDATA },
    { ST_INITIALIZING, EV_VISIONFAIL, ST_VISIONFAIL },
    { ST_MOREDATA, EV_CONVERGED, ST_READY },
    { ST_MOREDATA, EV_VISIONFAIL, ST_VISIONFAIL },
    { ST_MOREDATA, EV_FAIL, ST_FAIL },
    { ST_MOREDATA, EV_FAIL_TIMEOUT, ST_FAIL },
    { ST_READY, EV_TAP, ST_MEASURE },
    { ST_READY, EV_VISIONFAIL, ST_VISIONFAIL },
    { ST_READY, EV_FASTFAIL, ST_FASTFAIL },
    { ST_READY, EV_FAIL, ST_FAIL },
    { ST_MEASURE, EV_ALIGN, ST_ALIGN },
    { ST_MEASURE, EV_TAP, ST_FINISHED },
    { ST_MEASURE, EV_SPEEDWARNING, ST_SLOWDOWN },
    { ST_MEASURE, EV_VISIONFAIL, ST_VISIONFAIL },
    { ST_MEASURE, EV_FASTFAIL, ST_FASTFAIL },
    { ST_MEASURE, EV_FAIL, ST_FAIL },
    { ST_ALIGN, EV_TAP, ST_FINISHED },
    { ST_ALIGN, EV_VISIONFAIL, ST_VISIONFAIL },
    { ST_ALIGN, EV_FASTFAIL, ST_FASTFAIL },
    { ST_ALIGN, EV_FAIL, ST_FAIL },
    { ST_FINISHED, EV_PAUSE, ST_FINISHED },
    { ST_VISIONFAIL, EV_FAIL_EXPIRED, ST_INITIALIZING },
    { ST_FASTFAIL, EV_FAIL_EXPIRED, ST_INITIALIZING },
    { ST_FAIL, EV_FAIL_EXPIRED, ST_INITIALIZING },
    { ST_ANY, EV_PAUSE, ST_STARTUP },
    { ST_ANY, EV_CANCEL, ST_STARTUP }
};

#define TRANS_COUNT (sizeof(transitions) / sizeof(*transitions))

- (void) transitionToState:(int)newState
{
    if(currentState == newState) return;
    statesetup oldSetup = setups[currentState];
    statesetup newSetup = setups[newState];
    
/*    if(!oldsetup.plugins && newSetup.plugins)
        [self startPlugins];
    if(!oldSetup.measuring && newSetup.measuring)
        [self startMeasuring];
    if(oldSetup.measuring && !newSetup.measuring)
        [self stopMeasuring];
    if(oldSetup.plugins && !newSetup.plugins)
        [self shutdownPlugins];
    if(!oldSetup.crosshairs && newSetup.crosshairs)
        [self showCrosshairs];
    if(!oldSetup.target && newSetup.target)
        [self showTarget]
    if(oldSetup.crosshairs && !newSetup.crosshairs)
        [self hideCrosshairs];
    if(oldSetup.target && !newSetup.target)
        [self hideTarget];*/
    currentState = newState;

    [self showIcon:newSetup.icon];
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

const double stateTimeout = 5.;
/*
- (void)handleFilterEventWithCode:(int)code converged:(bool)converged steady:(bool)steady aligned:(bool)aligned speed_warning:(bool)speed_warning vision_failure:(bool)vision_failure speed_failure:(bool)speed_failure other_failure:(bool)other_failure
{
    double currentTime = CACurrentMediaTime();
    if(speed_failure) [self handleStateEvent:EV_FASTFAIL];
    else if(other_failure) [self handleStateEvent:EV_FAIL];
    else if(vision_failure) [self handleStateEvent:EV_VISIONFAIL];
    else lastFailTime = currentTime;

    double time_in_state = currentTime - lastTransitionTime;
    if(converged) [self handleStateEvent:EV_CONVERGED];
    else if(steady && time_in_state > stateTimeout) [self handleStateEvent:EV_CONVERGE_TIMEOUT];
    else if(time_in_state > stateTimeout) [self handleStateEvent:EV_FAIL_TIMEOUT];
  
    double time_since_fail = currentTime - lastFailTime;
    if(time_since_fail > 1.) [self handleStateEvent:EV_FAIL_EXPIRED];
    
    if(speed_warning) [self handleStateEvent:EV_SPEEDWARNING];
    
    if(aligned) [self handleStateEvent:EV_ALIGN];
}*/

- (void)viewWillAppear:(BOOL)animated
{
    [self.navigationController setToolbarHidden:NO animated:animated];
    [super viewWillAppear:animated];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    //register to receive notifications of pause/resume events
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    
	self.isCapturingData = NO;
    self.isMeasuring = NO;
    self.isMeasurementComplete = NO;
    useLocation = [LOCATION_MANAGER isLocationAuthorized] && [[NSUserDefaults standardUserDefaults] boolForKey:PREF_ADD_LOCATION];
	
    NSMutableArray *navigationArray = [[NSMutableArray alloc] initWithArray: self.navigationController.viewControllers];
    
    if(navigationArray.count > 1) 
    {
        [navigationArray removeObjectAtIndex: 1];  // remove Choose Type VC from nav array, so back button goes to history instead
        self.navigationController.viewControllers = navigationArray;
    }
        
    [self showMessage:@"Make sure the camera can see some stationary objects. Avoid pointing it at featureless areas, like blank walls."
            withTitle:@"Tip"
             autoHide:YES];
    
    [self setLocationButtonState];
}

- (void)viewDidUnload
{
	NSLog(@"viewDidUnload");
	[self setLblDistance:nil];
	[self setLblInstructions:nil];
	[self setBtnBegin:nil];
	[self setVideoPreviewView:nil];
    [self setBtnPageCurl:nil];
    [self setBtnBegin:nil];
    [self setInstructionsBg:nil];
    [self setDistanceBg:nil];
    [self setBtnSave:nil];
    [self setLocationButton:nil];
    [self setStatusIcon:nil];
	[super viewDidUnload];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
	NSLog(@"MEMORY WARNING");
}

- (void) viewDidAppear:(BOOL)animated
{
    [TMAnalytics logEvent:@"View.NewMeasurement"];
    [super viewDidAppear:animated];
    
    crosshairsLayer.hidden = NO;
    [crosshairsLayer needsLayout];
    [self handleResume];
}

- (void) viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    [repeatingTimer invalidate];
    targetLayer.delegate = nil;
    crosshairsLayer.delegate = nil;
    [self cancelMeasuring];
    [self performSelectorInBackground:@selector(endSession) withObject:nil];
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

- (void)endSession
{
    [SESSION_MANAGER endSession];
}

- (void)handlePause
{
	NSLog(@"handlePause");
	[self cancelMeasuring];
}

- (void)handleResume
{
	NSLog(@"handleResume");
    [self handleStateEvent:EV_RESUME];
	
	//watch inertial sensors on background thread
//	[self performSelectorInBackground:(@selector(watchDeviceMotion)) withObject:nil];
    if (![SESSION_MANAGER isRunning]) [SESSION_MANAGER startSession]; //might not be running due to app pause
    [self performSelectorInBackground:@selector(setupVideoPreview) withObject:nil]; //background thread helps UI load faster
    if (!self.isMeasurementComplete) [self prepareForMeasuring];
}

//handles button tap event
- (IBAction)handleButtonTap:(id)sender
{
	[self toggleMeasuring];
    [self handleStateEvent:EV_TAP];
}

- (void)setupVideoPreview
{
    NSLog(@"setupVideoPreview");

    self.videoPreviewView.clipsToBounds = YES;
    SESSION_MANAGER.videoPreviewLayer.videoGravity = AVLayerVideoGravityResizeAspect; //fill view, cropping if necessary
    
    [self setupVideoPreviewFrame];
    [self.videoPreviewView.layer addSublayer:SESSION_MANAGER.videoPreviewLayer];
    
    float circleRadius = 40.;
    crosshairsDelegate = [[TMCrosshairsLayerDelegate alloc] initWithRadius:circleRadius];
    crosshairsLayer = [CALayer new];
    [crosshairsLayer setDelegate:crosshairsDelegate];
    crosshairsLayer.hidden = NO;
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
}

- (void) setupVideoPreviewFrame
{
    NSLog(@"setupVideoPreviewFrame");

    if ([SESSION_MANAGER.videoPreviewLayer respondsToSelector:@selector(connection)])
    {
        if ([SESSION_MANAGER.videoPreviewLayer.connection isVideoOrientationSupported])
        {
            [SESSION_MANAGER.videoPreviewLayer.connection setVideoOrientation:self.interfaceOrientation];
        }
    }
    else
    {
        // Deprecated in 6.0; here for backward compatibility
        if ([SESSION_MANAGER.videoPreviewLayer isOrientationSupported])
        {
            [SESSION_MANAGER.videoPreviewLayer setOrientation:self.interfaceOrientation];
        }
    }
    
    CGRect videoRect = self.videoPreviewView.bounds;
    SESSION_MANAGER.videoPreviewLayer.frame = videoRect;
}

- (void)prepareForMeasuring
{
    NSLog(@"prepareForMeasuring");
    
    [self.btnBegin setTitle:@"Begin Measuring"];
    self.btnBegin.enabled = YES;
        
    self.btnPageCurl.enabled = YES;
    self.navigationItem.hidesBackButton = NO;
    
    [self hideDistanceLabel];
    
    crosshairsLayer.hidden = NO;
    [crosshairsLayer needsLayout];
    
    //make sure we have up to date location data
    if (useLocation) [LOCATION_MANAGER startLocationUpdates];
    
    newMeasurement = [TMMeasurement getNewMeasurement];

    CLLocation *loc = [LOCATION_MANAGER getStoredLocation];
    
    [CORVIS_MANAGER
     setupPluginsWithFilter:true
     withCapture:false
     withReplay:false
     withLocationValid:loc ? true : false
     withLatitude:loc ? loc.coordinate.latitude : 0
     withLongitude:loc ? loc.coordinate.longitude : 0
     withAltitude:loc ? loc.altitude : 0
     withStatusCallback:^(bool measurement_active, float x, float stdx, float y, float stdy, float z, float stdz, float path, float stdpath, float rx, float stdrx, float ry, float stdry, float rz, float stdrz, float orientx, float orienty, int code, bool converged, bool steady, bool aligned, bool speed_warning, bool vision_failure, bool speed_failure, bool other_failure) {
         
         double currentTime = CACurrentMediaTime();
         if(speed_failure) {
             [self handleStateEvent:EV_FASTFAIL];
             lastFailTime = currentTime;
         } else if(other_failure) {
             [self handleStateEvent:EV_FAIL];
             lastFailTime = currentTime;
         } else if(vision_failure) {
             [self handleStateEvent:EV_VISIONFAIL];
             lastFailTime = currentTime;
         }
         double time_in_state = currentTime - lastTransitionTime;
         if(converged) [self handleStateEvent:EV_CONVERGED];
         else if(steady && time_in_state > stateTimeout) [self handleStateEvent:EV_CONVERGE_TIMEOUT];
         else if(time_in_state > stateTimeout) [self handleStateEvent:EV_FAIL_TIMEOUT];
         
         double time_since_fail = currentTime - lastFailTime;
         if(time_since_fail > 1.) [self handleStateEvent:EV_FAIL_EXPIRED];
         
         if(speed_warning) [self handleStateEvent:EV_SPEEDWARNING];
         
         if(aligned) [self handleStateEvent:EV_ALIGN];
         fprintf(stderr, "current state is %d\n", currentState);
         
         [self updateOverlayWithX:orientx withY:orienty];
         
         if(measurement_active) [self updateMeasurementDataWithX:x
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
    self.isCapturingData = YES;
}

- (void)startMeasuring
{
    NSLog(@"startMeasuring");
    
	if(self.isMeasuring)
    {
        NSLog(@"Cannot start measuring. Measuring already in progress");
        return;
    }
	
    [TMAnalytics
     logEvent:@"Measurement.Start"
     withParameters:[NSDictionary dictionaryWithObjectsAndKeys:useLocation ? @"Yes" : @"No", @"WithLocation", nil]
     ];
    
    [self.btnBegin setTitle:@"Stop Measuring"];
    
    [self hideMessage];
    [self showDistanceLabel];
    
    [self updateMeasurementDataWithX:0 stdx:0 y:0 stdy:0 z:0 stdz:0 path:0 stdpath:0 rx:0 stdrx:0 ry:0 stdry:0 rz:0 stdrz:0];
    
    self.btnSave.enabled = NO;
    self.btnPageCurl.enabled = NO;
            
    distanceMeasured = 0;

    [CORVIS_MANAGER startMeasurement];
    
    self.isMeasuring = YES;
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
    self.lblDistance.text = [NSString stringWithFormat:@"%@", [newMeasurement getFormattedDistance:[newMeasurement getPrimaryMeasurementDist]]];
}

- (void)stopMeasuring
{
    NSLog(@"stopMeasuring");
    
	if(!self.isMeasuring)
	{
        NSLog(@"Cannot stop measuring. Measuring not started");
    }
    
    [CORVIS_MANAGER stopMeasurement];

    [TMAnalytics logEvent:@"Measurement.Stop"];
    
    self.navigationItem.hidesBackButton = YES;
    self.btnBegin.enabled = NO;
    self.locationButton.enabled = NO;
    
    [self shutdownDataCapture];
    targetLayer.hidden = YES;
    crosshairsLayer.hidden = YES;
    [self.videoPreviewView.layer needsLayout];
    
    self.isMeasurementComplete = YES;
    self.navigationItem.hidesBackButton = NO;
    self.btnSave.enabled = YES;
    self.btnPageCurl.enabled = YES;
    self.locationButton.enabled = YES;
    self.isMeasuring = NO;
}

- (void)cancelMeasuring
{
    NSLog(@"cancelMeasuring");
    
    if (self.isCapturingData)
    {
        [self shutdownDataCapture];
        targetLayer.hidden = YES;
        crosshairsLayer.hidden = YES;
        [self.videoPreviewView.layer needsLayout];
        self.isMeasuring = NO;
    }
}

- (void)shutdownDataCapture
{
    NSLog(@"shutdownDataCapture:begin");
    
    [VIDEOCAP_MANAGER stopVideoCap];
    [MOTIONCAP_MANAGER stopMotionCap];
    
    [NSThread sleepForTimeInterval:0.2]; //hack to prevent CorvisManager from receiving a video frame after plugins have stopped.
    
    [CORVIS_MANAGER stopPlugins];
    [CORVIS_MANAGER teardownPlugins];
    
    self.isCapturingData = NO;
    
    NSLog(@"shutdownDataCapture:end");
}

- (void)toggleMeasuring
{
	if(!self.isMeasuring){
        [self startMeasuring];
	} else {
        [self stopMeasuring];
	}
}

- (void)saveMeasurement
{
    NSLog(@"saveMeasurement");
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
    
    if (locationObj.syncPending) {
        [locationObj
         postToServer:^(int transId)
         {
             NSLog(@"Post location success callback");
             locationObj.syncPending = NO;
             [DATA_MANAGER saveContext];
             [self postMeasurement];
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

}

//this method is called by the timer object every tick
- (void)updateOverlayWithX:(float)x withY:(float)y
{
//    fprintf(stderr, "projected orientation is %f %f\n", x, y);
    
    float centerX = self.videoPreviewView.frame.size.width / 2 - (y * self.videoPreviewView.frame.size.width);
    float centerY = self.videoPreviewView.frame.size.height / 2 + (x * self.videoPreviewView.frame.size.width);

    //constrain target location to bounds of frame
    centerX = centerX > self.videoPreviewView.frame.size.width ? self.videoPreviewView.frame.size.width : centerX;
    centerX = centerX < 0 ? 0 : centerX;
    centerY = centerY > self.videoPreviewView.frame.size.height ? self.videoPreviewView.frame.size.height : centerY;
    centerY = centerY < 0 ? 0 : centerY;

    float radius = targetLayer.frame.size.height / 2;
    targetLayer.frame = CGRectMake(centerX - radius, centerY - radius, radius * 2, radius * 2);
    targetLayer.hidden = NO;
    [targetLayer needsLayout];
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
        case ICON_GREEN:
            self.statusIcon.image = [UIImage imageNamed:@"go_small"];
            break;
            
        case ICON_YELLOW:
            self.statusIcon.image = [UIImage imageNamed:@"caution_small"];
            break;
            
        case ICON_RED:
            self.statusIcon.image = [UIImage imageNamed:@"stop_small"];
            break;
            
        default:
            break;
    }
    
    self.statusIcon.hidden = NO;
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
        
        dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (fadeTime + delayTime) * NSEC_PER_SEC), dispatch_get_current_queue(), ^{
            self.navigationController.navigationBar.topItem.title = @"";
        });
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
}

- (void)hideDistanceLabel
{
    self.distanceBg.hidden = YES;
    self.lblDistance.hidden = YES;
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
//	NSLog(@"Watching device motion");
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

- (void) handleBump
{
	int currTime = CFAbsoluteTimeGetCurrent(); //time in sec
	int timeElapsed = currTime - lastBump; //since last bump
	
	if(timeElapsed >= 1) { //allow another bump 1s after last bump
		NSLog(@"Bump");
		lastBump = currTime;
		
		[self toggleMeasuring];
	} 
}

- (IBAction)handlePageCurl:(id)sender
{
    [TMAnalytics logEvent:@"Measurement.ViewOptions.NewMeasurement"];
}

- (IBAction)handleSaveButton:(id)sender
{
    [self saveMeasurement];
    [self performSegueWithIdentifier:@"toResult" sender:self.btnSave];
}

- (IBAction)handleLocationButton:(id)sender {
    NSLog(@"Location button");
    
    useLocation = !useLocation;
    
    [self setLocationButtonState];
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
        dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^
        {
           [SESSION_MANAGER endSession];
        });
        
        TMOptionsVC *optionsVC = [segue destinationViewController];
        optionsVC.theMeasurement = newMeasurement;
        
        [[segue destinationViewController] setDelegate:self];
    }
}

- (void)didDismissOptions
{
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        [SESSION_MANAGER startSession];
    });
    [self updateDistanceLabel];
}

- (void)setLocationButtonState
{
    if(useLocation)
    {
        self.locationButton.image = [UIImage imageNamed:@"ComposeSheetLocationArrowActive.png"];
    }
    else
    {
        self.locationButton.image = [UIImage imageNamed:@"ComposeSheetLocationArrow.png"];
        if (!locationAuthorized) self.locationButton.enabled = NO;
    }
}

@end
