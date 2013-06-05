//
//  TMViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "INMapVC.h"

#define METERS_PER_DEGREE 111120.

@implementation INMapVC
{
    MBProgressHUD *progressView;
    
    double lastTransitionTime;
    double lastFailTime;
    int filterFailCode;
    bool isAligned;
    bool isVisionWarning;
    
    float mapHeading;
    float currentHeading;
    
    double startLat, startLon, startOrient;
    double cosLat, sinOrient, cosOrient;
    
    MKPolyline* polyline;
    NSMutableArray* waypoints;
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
    bool crosshairs;
    bool target;
    bool showDistance;
    bool features;
    bool progress;
    const char *title;
    const char *message;
    bool autohide;
} statesetup;

statesetup setups[] =
{
    { ST_STARTUP, ICON_YELLOW,          true,   false,  false,  false,  false,  false,  false,  false,  "Initializing", "Move the device around very slowly and smoothly.", false},
    { ST_FOCUS, ICON_YELLOW,            true,   false,  false,  false,  false,  false,  false,  false,  "Focusing",     "Point the camera at an area with lots of visual detail, and tap the screen to lock the focus.", false},
    { ST_FIRSTFOCUS, ICON_YELLOW,       true,   false,  false,  false,  false,  false,  false,  false,  "Focusing",     "We need to calibrate your device just once. Point the camera at something well-lit and visually complex, like a bookcase, and tap to lock the focus.", false},
    { ST_FIRSTCALIBRATION, ICON_YELLOW, false,  true,   false,  false,  false,  false,  true,   true,   "Calibrating",  "Please move the device around very slowly to calibrate it. Slowly rotate the device from side to side as you go.", false},
    { ST_CALIB_ERROR, ICON_YELLOW,      false,  true,   false,  false,  false,  false,  true,   true,   "Calibrating",  "This might take a couple of attempts. Be sure to move very slowly, and try rotating your device from side to side. Code %04x.", false},
    { ST_INITIALIZING, ICON_YELLOW,     false,  true,   false,  false,  false,  false,  true,   true,   "Initializing", "Move the device around very slowly and smoothly.", false},
    { ST_MOREDATA, ICON_YELLOW,         false,  true,   false,  false,  false,  false,  true,   true,   "Initializing", "Move the device around very slowly and smoothly.", false },
    { ST_READY, ICON_GREEN,             false,  true,   false,  true,   false,  true,   true,   false,  "Ready",        "Move the device to one end of the thing you want to measure, and tap the screen to start.", false },
    { ST_MEASURE, ICON_GREEN,           false,  true,   true,   true,   false,  true,   true,   false,  "Measuring",    "Move the device to the other end of what you're measuring. We'll show you how far it moved.", false },
    { ST_MEASURE_STEADY, ICON_GREEN,    false,  true,   true,   true,   false,  true,   true,   false,  "Measuring",    "Tap the screen to finish.", false },
    { ST_ALIGN, ICON_GREEN,             true,   false,  false,  false,  false,  true,   false,  false,  "Finished",     "Looks good.", false },
    { ST_VISIONWARN, ICON_YELLOW,       true,   false,  false,  false,  false,  true,   false,  false,  "Finished",     "It was hard to see at times, so it might be inaccurate.", false },
    { ST_FINISHED, ICON_GREEN,          true,   false,  false,  false,  false,  true,   false,  false,  "Finished",     "Looks good.", false },
    { ST_VISIONFAIL, ICON_RED,          false,  true,   false,  false,  false,  true,   false,  false,  "Try again",    "Sorry, I can't see well enough right now. Make sure the area is well lit. Error code %04x.", false },
    { ST_FASTFAIL, ICON_RED,            false,  true,   false,  false,  false,  true,   false,  false,  "Try again",    "Sorry, that didn't work. Try to move slowly and smoothly. Error code %04x.", false },
    { ST_FAIL, ICON_RED,                false,  true,   false,  false,  false,  true,   false,  false,  "Try again",    "Sorry, we need to try that again. If that doesn't work send error code %04x to support@realitycap.com.", false },
    { ST_SLOWDOWN, ICON_YELLOW,         false,  true,   true,   true,   false,  true,   true,   false,  "Measuring",    "Slow down please. You'll get the most accurate results by moving very slowly and smoothly.", false }
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
        [self stopNavigating];
    if(!oldSetup.datacapture && newSetup.datacapture)
        [self startDataCapture];
    if(!oldSetup.measuring && newSetup.measuring)
        [self startNavigating];
    if(oldSetup.measuring && !newSetup.measuring)
        [self stopNavigating];
    if(oldSetup.datacapture && !newSetup.datacapture)
        [self shutdownDataCapture];
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
        
    mapHeading = currentHeading = 0;
    
    waypoints = [NSMutableArray new];
    
    self.mapView.delegate = self;
    
    //setup screen tap detection
    UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapGesture:)];
    tapGesture.numberOfTapsRequired = 1;
    [self.mapView addGestureRecognizer:tapGesture];
    
    [self rotateArrowByDegrees:-45];
}

- (void)viewDidUnload
{
	LOGME
	[self setLblInstructions:nil];
    [self setInstructionsBg:nil];
    [self setStatusIcon:nil];
    [self setMapView:nil];
    [self setArrowImage:nil];
    [self setMapView:nil];
	[super viewDidUnload];
}

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
}

- (void) viewDidAppear:(BOOL)animated
{
    LOGME
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
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super viewWillDisappear:animated];
}

- (void)handlePause
{
	LOGME
    [self handleStateEvent:EV_PAUSE];
    
    LOCATION_MANAGER.delegate = nil;
    
    [self endAVSessionInBackground];
}

- (void)handleResume
{
	LOGME
    LOCATION_MANAGER.delegate = self;
    [LOCATION_MANAGER startLocationUpdates];
    
    if (![SESSION_MANAGER isRunning]) [SESSION_MANAGER startSession]; //might not be running due to app pause
    
    if([RCCalibration hasCalibrationData]) {
        [self handleStateEvent:EV_RESUME];
    } else {
        [self handleStateEvent:EV_FIRSTTIME];
    }
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

-(void) handleTapGesture:(UIGestureRecognizer *) sender {
    if (sender.state != UIGestureRecognizerStateEnded) return;
    if(isVisionWarning)
        [self handleStateEvent:EV_TAP_WARNING];
    if(!isAligned)
        [self handleStateEvent:EV_TAP_UNALIGNED];
    [self handleStateEvent:EV_TAP]; //always send the tap - align ignores it and others might need it
}

- (IBAction)handleLocationButton:(id)sender
{
    [self centerMapOnCurrentLocation];
    
    // for testing line drawing
//    CLLocationCoordinate2D coord = [self getCurrentMapCenter];
//    CLLocation* location = [[CLLocation alloc] initWithLatitude:coord.latitude longitude:coord.longitude];
//    
//    if (waypoints.count == 0)
//    {
//        [waypoints addObject:location];
//    }
//    else
//    {
//        CLLocation* lastLoc = [waypoints objectAtIndex:waypoints.count - 1];
//        if ([location distanceFromLocation:lastLoc] > 1) [waypoints addObject:location];
//        [self updatePathOverlay];
//    }    
}

- (void) centerMapOnCurrentLocation
{
    CLLocation *storedLocation = [LOCATION_MANAGER getStoredLocation];
    if(storedLocation) [self centerMapOnLocation:storedLocation];
}

- (void) centerMapOnLocation:(CLLocation*)location
{
    double zoomLevel = 100; //meters
        
    if(location)
    {
        CLLocationCoordinate2D center = CLLocationCoordinate2DMake(location.coordinate.latitude, location.coordinate.longitude);
        [self.mapView setRegion:MKCoordinateRegionMakeWithDistance(center, zoomLevel, zoomLevel) animated:YES];
        [[[self mapView] userLocation] setCoordinate:center];
    }
}

- (CLLocationCoordinate2D) getCurrentMapCenter
{
    return self.mapView.centerCoordinate;
}

- (void) rotateArrowByDegrees:(float)degrees
{
    float radians = degrees * (M_PI / 180); 
    self.arrowImage.transform = CGAffineTransformRotate(self.arrowImage.transform, radians);
}

- (void) rotateMapByDegrees:(float)degrees
{
    mapHeading = mapHeading + degrees;
    if (mapHeading > 360) mapHeading = mapHeading - 360;
    
    float radians = -degrees * (M_PI / 180); 
    self.mapView.transform = CGAffineTransformRotate(self.mapView.transform, radians);
}

- (void) rotateMapToHeading:(float)heading
{
    if (heading > 360) return;
    float degreesDifference = heading - mapHeading;
    [self rotateMapByDegrees:degreesDifference];
}

- (void)startDataCapture
{
    LOGME
    
    CLLocation *loc = [LOCATION_MANAGER getStoredLocation];
    [LOCATION_MANAGER startHeadingUpdates];
    
    __weak INMapVC* weakSelf = self;
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

- (void)initializeNavigation
{
    CLLocationCoordinate2D loc = [self getCurrentMapCenter];
    //TODO: get initial orientation from compass
  
    // add start point to waypoints
    [waypoints addObject:[[CLLocation alloc] initWithLatitude:loc.latitude longitude:loc.longitude]];
    
    startLat = loc.latitude;
    startLon = loc.longitude;
    startOrient = currentHeading;
    cosLat = cos(startLat * M_PI / 180.);
    sinOrient = sin(startOrient);
    cosOrient = cos(startOrient);
}

- (void)updateNavigationWithX:(float)x withY:(float)y
{
    double east = cosOrient * x - sinOrient * y;
    double north = sinOrient * x + cosOrient * y;
    double lon = startLon + east / (cosLat * METERS_PER_DEGREE);
    double lat = startLat + north / METERS_PER_DEGREE;
    
    CLLocation* location = [[CLLocation alloc] initWithLatitude:lat longitude:lon];
    [self centerMapOnLocation:location];
    
    // add a waypoint to the array if it's at least 1m from the last waypoint in the array
    if (waypoints.count == 0 || [location distanceFromLocation:[waypoints objectAtIndex:waypoints.count - 1]] > 1)
    {
        [waypoints addObject:location];
        [self updatePathOverlay];
    }
}

- (void)startNavigating
{
    LOGME
    
    [self initializeNavigation];
    [CORVIS_MANAGER startMeasurement];
}

- (void)updateMeasurementDataWithX:(float)x stdx:(float)stdx y:(float)y stdy:(float)stdy z:(float)z stdz:(float)stdz path:(float)path stdpath:(float)stdpath rx:(float)rx stdrx:(float)stdrx ry:(float)ry stdry:(float)stdry rz:(float)rz stdrz:(float)stdrz
{
    [self updateNavigationWithX:x withY:y];
}

- (void)stopNavigating
{
    LOGME
    
    [CORVIS_MANAGER stopMeasurement];
    [CORVIS_MANAGER saveDeviceParameters];
}

- (void)shutdownDataCapture
{
    LOGME
    
    [LOCATION_MANAGER stopHeadingUpdates];
    [VIDEOCAP_MANAGER stopVideoCap];
    [MOTIONCAP_MANAGER stopMotionCap];
    
    [NSThread sleepForTimeInterval:0.2]; //hack to prevent CorvisManager from receiving a video frame after plugins have stopped.
    
    [CORVIS_MANAGER stopPlugins];
    [CORVIS_MANAGER teardownPlugins];
}

- (void)updatePathOverlay
{
    int numPoints = [waypoints count];
    if (numPoints > 1)
    {
        CLLocationCoordinate2D* coords = malloc(numPoints * sizeof(CLLocationCoordinate2D));
        for (int i = 0; i < numPoints; i++)
        {
            CLLocation* location = [waypoints objectAtIndex:i];
            coords[i] = location.coordinate;
        }
        
        if (polyline) [self.mapView removeOverlay:polyline];
        
        polyline = [MKPolyline polylineWithCoordinates:coords count:numPoints];
        free(coords);
        
        [self.mapView addOverlay:polyline];
        [self.mapView setNeedsDisplay];
    }
}

- (MKOverlayView*)mapView:(MKMapView*)theMapView viewForOverlay:(id <MKOverlay>)overlay
{
    MKPolylineView* lineView = [[MKPolylineView alloc] initWithPolyline:overlay];
    lineView.strokeColor = [UIColor colorWithRed:0 green:0 blue:1. alpha:.4];
    lineView.lineWidth = 4;
    lineView.opaque = NO;
    return lineView;
}

- (void) locationManager:(CLLocationManager *)manager didUpdateHeading:(CLHeading *)newHeading
{
    currentHeading = newHeading.trueHeading;
    [self rotateMapToHeading:currentHeading];
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

- (void)showIcon:(IconType)type
{
    switch (type) {
        case ICON_HIDDEN:
            self.statusIcon.hidden = YES;
            break;

        case ICON_GREEN:
            self.statusIcon.image = [UIImage imageNamed:@"go"];
            self.statusIcon.hidden = NO;
            break;
            
        case ICON_YELLOW:
            self.statusIcon.image = [UIImage imageNamed:@"caution"];
            self.statusIcon.hidden = NO;
            break;
            
        case ICON_RED:
            self.statusIcon.image = [UIImage imageNamed:@"stop"];
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

@end
