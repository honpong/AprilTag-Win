//
//  TMViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "INMapVC.h"

#define METERS_PER_DEGREE 111120.

typedef enum
{
    ICON_HIDDEN, ICON_RED, ICON_YELLOW, ICON_GREEN
} IconType;

@implementation INMapVC
{
    MBProgressHUD *progressView;
    
    double lastTransitionTime;
    double lastFailTime;
    int filterFailCode;
    bool isAligned;
    bool isVisionWarning;
    
    float currentHeading;
    
    double startLat, startLon, startOrient;
    double cosLat, sinOrient, cosOrient;
    
    GMSPolyline* polyline;
    GMSMutablePath* pathTraveled;
}

- (void)viewDidLoad
{
    LOGME
	[super viewDidLoad];
    
    currentHeading = 0;
    
    GMSCameraPosition *camera = [GMSCameraPosition cameraWithLatitude:37.7567379317 longitude:-122.425719146 zoom:13];
    
    self.mapView = [GMSMapView mapWithFrame:CGRectMake(0, 0, self.view.frame.size.width, self.view.frame.size.height) camera:camera];
    self.mapView.myLocationEnabled = NO;
    [self.view addSubview:self.mapView];
    [self.view sendSubviewToBack:self.mapView];
    
    //setup screen tap detection
    UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapGesture:)];
    tapGesture.numberOfTapsRequired = 1;
    [self.mapView addGestureRecognizer:tapGesture];
    
    [self rotateArrowByDegrees:-45];
    
    pathTraveled = [GMSMutablePath path];
    polyline = [GMSPolyline polylineWithPath:pathTraveled];
    polyline.strokeColor = [UIColor colorWithRed:0 green:0 blue:1 alpha:.4];
    polyline.strokeWidth = 5;
    polyline.geodesic = YES;
    polyline.map = self.mapView;

    [[RCVideoManager sharedInstance] setupWithSession:[RCAVSessionManager sharedInstance].session];
    [RCSensorFusion sharedInstance].delegate = self;
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
    [self handleResume];
}

- (void) viewWillDisappear:(BOOL)animated
{
    LOGME
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [super viewWillDisappear:animated];
}

- (void)handlePause
{
	LOGME
    [self stopNavigating];
    [RCLocationManager sharedInstance].delegate = nil;
    [self clearPathTraveled];    
}

- (void)handleResume
{
	LOGME
    [RCLocationManager sharedInstance].delegate = self;
    [[RCLocationManager sharedInstance] startLocationUpdates];
    [[RCLocationManager sharedInstance] startHeadingUpdates];
    
    [[RCAVSessionManager sharedInstance] startSession]; 
    
    if([RCCalibration hasCalibrationData]) {
        [self prepareForNavigation];
    } else {
//        [self startCalibration];
    }
}

-(void) handleTapGesture:(UIGestureRecognizer *) sender {
//    if (sender.state != UIGestureRecognizerStateEnded) return;
//    if(isVisionWarning)
//        [self handleStateEvent:EV_TAP_WARNING];
//    if(!isAligned)
//        [self handleStateEvent:EV_TAP_UNALIGNED];
//    [self handleStateEvent:EV_TAP]; //always send the tap - align ignores it and others might need it
}

- (IBAction)handleLocationButton:(id)sender
{
    [self centerMapOnCurrentLocation];
    
    // for testing line drawing
//    CLLocationCoordinate2D coord = [self getCurrentMapCenter];
//    CLLocation* location = [[CLLocation alloc] initWithLatitude:coord.latitude longitude:coord.longitude];
//    [self updatePathTraveled: location];
}

- (void) centerMapOnCurrentLocation
{
    CLLocation *storedLocation = [[RCLocationManager sharedInstance] getStoredLocation];
    if(storedLocation) [self centerMapOnLocation:storedLocation];
}

- (void) centerMapOnLocation:(CLLocation*)location
{
    if(location)
    {
        CLLocationCoordinate2D coords = CLLocationCoordinate2DMake(location.coordinate.latitude, location.coordinate.longitude);
        [self.mapView animateToLocation:coords];
        [self.mapView animateToZoom:100];
    }
}

- (CLLocationCoordinate2D) getCurrentMapCenter
{
    return self.mapView.camera.target;
}

- (void) rotateArrowByDegrees:(float)degrees
{
    float radians = degrees * (M_PI / 180); 
    self.arrowImage.transform = CGAffineTransformRotate(self.arrowImage.transform, radians);
}

- (void) rotateMapToHeading:(float)heading
{
    if (heading > 360 || heading < 0) return;
    [self.mapView animateToBearing:heading];
}

- (void) clearPathTraveled
{
    [pathTraveled removeAllCoordinates];
    [polyline setPath:pathTraveled];
}

- (void) prepareForNavigation
{
    LOGME
    [[RCSensorFusion sharedInstance] startSensorFusion:[[RCLocationManager sharedInstance] getStoredLocation]];
}

- (void) sensorFusionDidUpdate:(RCSensorFusionData*)data
{
//    bool speed_warning = false;
//    bool speed_failure = false;
//    bool vision_failure = false;
//    bool other_failure = false;
//    
//    filterFailCode = data.status.statusCode;
//    double currentTime = CACurrentMediaTime();
//    if(speed_failure) {
//        [self handleStateEvent:EV_FASTFAIL];
//        lastFailTime = currentTime;
//    } else if(other_failure) {
//        [self handleStateEvent:EV_FAIL];
//        lastFailTime = currentTime;
//    } else if(vision_failure) {
//        [self handleStateEvent:EV_VISIONFAIL];
//        lastFailTime = currentTime;
//    }
//    if(lastFailTime == currentTime) {
//        //in case we aren't changing states, update the error message
//        NSString *message = [NSString stringWithFormat:[NSString stringWithCString:setups[currentState].message encoding:NSASCIIStringEncoding], filterFailCode];
//        [self showMessage:message withTitle:[NSString stringWithCString:setups[currentState].title encoding:NSASCIIStringEncoding] autoHide:setups[currentState].autohide];
//    }
//    double time_in_state = currentTime - lastTransitionTime;
//    [self updateProgress:data.status.initializationProgress];
//    if(data.status.initializationProgress >= 1.) {
//        if(currentState == ST_FIRSTCALIBRATION || currentState == ST_CALIB_ERROR) {
//            [self stopNavigating];
//        }
//        [self handleStateEvent:EV_CONVERGED];
//    }
//    if(data.status.isSteady && time_in_state > stateTimeout) [self handleStateEvent:EV_STEADY_TIMEOUT];
//    
//    double time_since_fail = currentTime - lastFailTime;
//    if(time_since_fail > failTimeout) [self handleStateEvent:EV_FAIL_EXPIRED];
//    
//    if(speed_warning) [self handleStateEvent:EV_SPEEDWARNING];
//    else [self handleStateEvent:EV_NOSPEEDWARNING];
    
    [self updateNavigationWithX:data.transformation.translation.x withY:data.transformation.translation.y];
}

- (void) sensorFusionWarning:(int)code
{
    
}

- (void) sensorFusionError:(NSError *)error
{
    
}

- (void)updateNavigationWithX:(float)x withY:(float)y
{
    double east = cosOrient * x - sinOrient * y;
    double north = sinOrient * x + cosOrient * y;
    double lon = startLon + east / (cosLat * METERS_PER_DEGREE);
    double lat = startLat + north / METERS_PER_DEGREE;
    
    CLLocation* location = [[CLLocation alloc] initWithLatitude:lat longitude:lon];
    [self centerMapOnLocation:location];
    [self updatePathTraveled:location];
}

- (void) startNavigating
{
    LOGME
    
    CLLocationCoordinate2D loc = [self getCurrentMapCenter];
    //TODO: get initial orientation from compass
    
    // add start point
    [pathTraveled addCoordinate:loc];
    
    startLat = loc.latitude;
    startLon = loc.longitude;
    startOrient = currentHeading;
    cosLat = cos(startLat * M_PI / 180.);
    sinOrient = sin(startOrient);
    cosOrient = cos(startOrient);
    
    [[RCSensorFusion sharedInstance] markStart];
}

- (void) stopNavigating
{
    LOGME
    [[RCVideoManager sharedInstance] stopVideoCapture];
    [[RCMotionManager sharedInstance] stopMotionCapture];
    [[RCSensorFusion sharedInstance] stopSensorFusion];
    [self endAVSessionInBackground];
}

- (void)updatePathTraveled:(CLLocation*)location
{
    if (location == nil) return;

    CLLocation* prevLocation;
    
    if (pathTraveled.count > 0)
    {
        CLLocationCoordinate2D prevCoord = [pathTraveled coordinateAtIndex:pathTraveled.count - 1];
        prevLocation = [[CLLocation alloc] initWithLatitude:prevCoord.latitude longitude:prevCoord.longitude];
    }
    
    if (prevLocation == nil || [location distanceFromLocation:prevLocation] > .5)
    {
        [pathTraveled addCoordinate:location.coordinate];
        [polyline setPath:pathTraveled]; // seems like this shouldn't be necessary, but it is
    }
}

- (void) locationManager:(CLLocationManager *)manager didUpdateHeading:(CLHeading *)newHeading
{
    currentHeading = newHeading.trueHeading;
    [self rotateMapToHeading:currentHeading];
}

- (void) showProgressWithTitle:(NSString*)title
{
    progressView = [[MBProgressHUD alloc] initWithView:self.view];
    progressView.mode = MBProgressHUDModeAnnularDeterminate;
    [self.view addSubview:progressView];
    progressView.labelText = title;
    [progressView show:YES];
}

- (void) hideProgress
{
    [progressView hide:YES];
}

- (void) updateProgress:(float)progress
{
    [progressView setProgress:progress];
}

- (void) showIcon:(IconType)type
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

- (void) hideIcon
{
    self.statusIcon.hidden = YES;
}

- (void)showMessage:(NSString*)message withTitle:(NSString*)title autoHide:(BOOL)hide
{
    self.instructionsBg.hidden = NO;
    self.lblInstructions.hidden = NO;
    
    self.lblInstructions.text = message ? message : @"";    
    self.navigationController.navigationBar.topItem.title = title ? title : @"";
    
    self.instructionsBg.alpha = 0.7;
    self.lblInstructions.alpha = 1;
    
    if (hide)
    {
        int const delayTime = 5;
        int const fadeTime = 2;
        
        [self fadeOut:self.lblInstructions withDuration:fadeTime andWait:delayTime];
        [self fadeOut:self.instructionsBg withDuration:fadeTime andWait:delayTime];
    }
}

- (void) hideMessage
{
    [self fadeOut:self.lblInstructions withDuration:0.5 andWait:0];
    [self fadeOut:self.instructionsBg withDuration:0.5 andWait:0];
    
    self.navigationController.navigationBar.topItem.title = @"";
}

-(void) fadeOut:(UIView*)viewToDissolve withDuration:(NSTimeInterval)duration andWait:(NSTimeInterval)wait
{
    [UIView beginAnimations: @"Fade Out" context:nil];
    
    // wait for time before begin
    [UIView setAnimationDelay:wait];
    
    // duration of animation
    [UIView setAnimationDuration:duration];
    viewToDissolve.alpha = 0.0;
    [UIView commitAnimations];
}

-(void) fadeIn:(UIView*)viewToFade withDuration:(NSTimeInterval)duration withAlpha:(float)alpha andWait:(NSTimeInterval)wait
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

-(void) fadeIn:(UIView*)viewToFade withDuration:(NSTimeInterval)duration andWait:(NSTimeInterval)wait
{
    [self fadeIn:viewToFade withDuration:duration withAlpha:1.0 andWait:wait];
}

- (void) endAVSessionInBackground
{
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        [[RCAVSessionManager sharedInstance] endSession];
    });
}

@end
