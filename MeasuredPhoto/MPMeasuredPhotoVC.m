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
    
    MBProgressHUD *progressView;
          
    double lastTransitionTime;
    double lastFailTime;
    int filterStatusCode;
    bool isAligned;
    
    RCFeaturePoint* lastPointTapped;
    
    RCSensorFusionData* sfData;
}
@synthesize toolbar;

static const double stateTimeout = 2.;
static const double failTimeout = 2.;

typedef enum
{
    ICON_HIDDEN, ICON_RED, ICON_YELLOW, ICON_GREEN
} IconType;

enum state { ST_STARTUP, ST_FIRSTFOCUS, ST_FIRSTCALIBRATION, ST_INITIALIZING, ST_MOREDATA, ST_READY, ST_MEASURE, ST_MEASURE_STEADY, ST_FINISHED, ST_FINISHEDPAUSE, ST_FINISHEDCALIB, ST_VISIONFAIL, ST_FASTFAIL, ST_FAIL, ST_ANY } currentState;
enum event { EV_RESUME, EV_FIRSTTIME, EV_CONVERGED, EV_STEADY_TIMEOUT, EV_VISIONFAIL, EV_FASTFAIL, EV_FAIL, EV_FAIL_EXPIRED, EV_TAP, EV_PAUSE, EV_CANCEL };

typedef struct { enum state state; enum event event; enum state newstate; } transition;

typedef struct
{
    enum state state;
    IconType icon;
    bool autofocus;
    bool videocapture;
    bool measuring;
    bool avSession;
    bool target;
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
    //                                  focus   capture measure session target  shwdstc shwtape ftrs    prgrs
    { ST_STARTUP, ICON_GREEN,           true,   false,  false,  true,   false,  false,  false,  false,  false,  "Startup",      "Move the device around very slowly and smoothly, while keeping some blue dots in sight.", false},
    { ST_FIRSTFOCUS, ICON_GREEN,        true,   false,  false,  true,   false,  false,  false,  false,  false,  "Focusing",     "We need to calibrate your device just once. Set it on a solid surface and tap to start.", false},
    { ST_FIRSTCALIBRATION, ICON_GREEN,  false,  true,   false,  true,   false,  false,  false,  true,   true,   "Calibrating",  "Make sure not to touch or bump the device or the surface it's on.", false},
    { ST_INITIALIZING, ICON_GREEN,      true,   true,   false,  true,   false,  false,  false,  true,   true,   "Initializing", "Move the device around very slowly and smoothly, while keeping some blue dots in sight.", false},
    { ST_MOREDATA, ICON_GREEN,          true,   true,   false,  true,   false,  false,  false,  true,   true,   "Initializing", "Move the device around very slowly and smoothly, while keeping some blue dots in sight.", false },
    { ST_READY, ICON_GREEN,             true,   true,   false,  true,   false,  true,   false,  true,   false,  "Ready",        "Move the device to one end of the thing you want to measure, and tap the screen to start.", false },
    { ST_MEASURE, ICON_GREEN,           false,  true,   true,   true,   false,  true,   true,   true,   false,  "Measuring",    "Move the device to the other end of what you're measuring. I'll show you how far the device moved.", false },
    { ST_MEASURE_STEADY, ICON_GREEN,    false,  true,   true,   true,   false,  true,   true,   true,   false,  "Measuring",    "Tap the screen to finish.", false },
    { ST_FINISHED, ICON_GREEN,          false,  true,   false,  false,  false,  true,   true,   true,   false,  "Finished",     "", false },
    { ST_FINISHEDPAUSE, ICON_GREEN,     false,  false,  false,  false,  false,  false,  true,   true,   false,  "Finished",     "", false },
    { ST_FINISHEDCALIB, ICON_GREEN,     false,  false,  false,  true,   false,  false,  false,  true,   false,  "Finished",     "", false },
    { ST_VISIONFAIL, ICON_RED,          true,   true,   false,  true,   false,  false,  false,  false,  false,  "Try again",    "Sorry, I can't see well enough to measure right now. Try to keep some blue dots in sight, and make sure the area is well lit. Error code %04x.", false },
    { ST_FASTFAIL, ICON_RED,            true,   true,   false,  true,   false,  false,  false,  false,  false,  "Try again",    "Sorry, that didn't work. Try to move very slowly and smoothly to get accurate measurements. Error code %04x.", false },
    { ST_FAIL, ICON_RED,                true,   true,   false,  true,   false,  false,  false,  false,  false,  "Try again",    "Sorry, we need to try that again. If that doesn't work send error code %04x to support@realitycap.com.", false },
};

static transition transitions[] =
{
    { ST_STARTUP, EV_TAP, ST_READY }, //ST_INITIALIZING },
    { ST_STARTUP, EV_FIRSTTIME, ST_FIRSTFOCUS }, //ST_FIRSTFOCUS },
    { ST_FIRSTFOCUS, EV_TAP, ST_FIRSTCALIBRATION },
    { ST_FIRSTCALIBRATION, EV_CONVERGED, ST_FINISHEDCALIB },
    { ST_INITIALIZING, EV_CONVERGED, ST_READY },
    { ST_INITIALIZING, EV_STEADY_TIMEOUT, ST_MOREDATA },
    { ST_MOREDATA, EV_CONVERGED, ST_READY },
    { ST_MOREDATA, EV_VISIONFAIL, ST_VISIONFAIL },
    { ST_MOREDATA, EV_FASTFAIL, ST_FASTFAIL },
    { ST_MOREDATA, EV_FAIL, ST_FAIL },
    { ST_READY, EV_TAP, ST_MEASURE },
    //{ ST_READY, EV_VISIONFAIL, ST_INITIALIZING },
    //{ ST_READY, EV_FASTFAIL, ST_INITIALIZING },
    //{ ST_READY, EV_FAIL, ST_INITIALIZING },
    { ST_MEASURE, EV_TAP, ST_FINISHED },
    { ST_MEASURE, EV_STEADY_TIMEOUT, ST_MEASURE_STEADY },
    { ST_MEASURE, EV_FASTFAIL, ST_FASTFAIL },
    { ST_MEASURE, EV_FAIL, ST_FAIL },
    { ST_MEASURE_STEADY, EV_TAP, ST_FINISHED },
    { ST_MEASURE_STEADY, EV_FASTFAIL, ST_FASTFAIL },
    { ST_MEASURE_STEADY, EV_FAIL, ST_FAIL },
    { ST_FINISHED, EV_PAUSE, ST_FINISHEDPAUSE },
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
    if(!oldSetup.progress && newSetup.progress)
        [self showProgressWithTitle:[NSString stringWithCString:newSetup.title encoding:NSASCIIStringEncoding]];
    currentState = newState;

    NSString *message = [NSString stringWithFormat:[NSString stringWithCString:newSetup.message encoding:NSASCIIStringEncoding], filterStatusCode];
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
    
    [self validateStateMachine];
    
    useLocation = [LOCATION_MANAGER isLocationAuthorized] && [[NSUserDefaults standardUserDefaults] boolForKey:PREF_ADD_LOCATION];
    
    //setup screen tap detection
    UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapGesture:)];
    tapGesture.numberOfTapsRequired = 1;
    [self.arView addGestureRecognizer:tapGesture];
    
    self.messageBackground.layer.cornerRadius = 10.;
    
    SENSOR_FUSION.delegate = self;
    [VIDEO_MANAGER setupWithSession:SESSION_MANAGER.session];
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
    NSArray* constraintHorz;
    NSArray* constraintVert;
    
    switch ([[UIDevice currentDevice] orientation])
    {
        case UIDeviceOrientationPortrait:
        {
            [self rotateUIByRadians:0];
            constraintHorz = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|-0-[toolbar]-0-|" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            constraintVert = [NSLayoutConstraint constraintsWithVisualFormat:@"[toolbar(100)]-0-|" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            break;
        }
        case UIDeviceOrientationPortraitUpsideDown:
        {
            [self rotateUIByRadians:M_PI];
            constraintHorz = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|-0-[toolbar]-0-|" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            constraintVert = [NSLayoutConstraint constraintsWithVisualFormat:@"|-0-[toolbar(100)]" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            break;
        }
        case UIDeviceOrientationLandscapeLeft:
        {
            [self rotateUIByRadians:M_PI_2];
            constraintHorz = [NSLayoutConstraint constraintsWithVisualFormat:@"|-0-[toolbar]-0-|" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            constraintVert = [NSLayoutConstraint constraintsWithVisualFormat:@"V:[toolbar(100)]-0-|" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            break;
        }
        case UIDeviceOrientationLandscapeRight:
        {
            [self rotateUIByRadians:-M_PI_2];
            constraintHorz = [NSLayoutConstraint constraintsWithVisualFormat:@"|-0-[toolbar]-0-|" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            constraintVert = [NSLayoutConstraint constraintsWithVisualFormat:@"V:|-0-[toolbar(100)]" options:NSLayoutFormatAlignAllCenterY metrics:nil views:NSDictionaryOfVariableBindings(toolbar)];
            break;
        }
        default:
            break;
    }
    
    if (constraintHorz && constraintVert)
    {
        [self.arView removeConstraints:self.arView.constraints];
        [self.arView addConstraints:constraintHorz];
        [self.arView addConstraints:constraintVert];
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
    if (![SESSION_MANAGER isRunning]) [SESSION_MANAGER startSession]; 
    
//    if([RCCalibration hasCalibrationData]) {
//        [self handleStateEvent:EV_RESUME];
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
    if (lastPointTapped)
    {
        [self.arView drawMeasurementBetweenPointA:pointTapped andPointB:lastPointTapped];
        lastPointTapped = nil;
        [self.arView clearSelectedFeatures];
    }
    else
    {
        lastPointTapped = pointTapped;
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
    } else if(error.other) {
        [self handleStateEvent:EV_FAIL];
        [SENSOR_FUSION resetSensorFusion];
    } else if(error.vision) {
        [self handleStateEvent:EV_VISIONFAIL];
    }
    if(lastFailTime == currentTime) {
        //in case we aren't changing states, update the error message
        NSString *message = [NSString stringWithFormat:[NSString stringWithCString:setups[currentState].message encoding:NSASCIIStringEncoding], filterStatusCode];
        [self showMessage:message withTitle:[NSString stringWithCString:setups[currentState].title encoding:NSASCIIStringEncoding] autoHide:setups[currentState].autohide];
    }
}

- (RCPoint *) calculateTapeStart:(RCSensorFusionData*)data
{
    NSMutableArray *sorted = [[NSMutableArray alloc] initWithCapacity:data.featurePoints.count];
    for(int i = 0; i < data.featurePoints.count; ++i) {
        RCFeaturePoint *pt = (RCFeaturePoint *)data.featurePoints[i];
        if(pt.initialized) {
            [sorted addObject:pt];
        }
    }
    [sorted sortUsingComparator:^NSComparisonResult(id a, id b) {
        RCFeaturePoint *pt1 = (RCFeaturePoint *)a;
        RCFeaturePoint *pt2 = (RCFeaturePoint *)b;
        if (pt1.depth.scalar > pt2.depth.scalar) {
            return (NSComparisonResult)NSOrderedDescending;
        }
        if (pt1.depth.scalar < pt2.depth.scalar) {
            return (NSComparisonResult)NSOrderedAscending;
        }
        return (NSComparisonResult)NSOrderedSame;
    }];
    //TODO: restrict this to only the close features to the starting point
    float median = ((RCFeaturePoint *)sorted[[sorted count]/2]).depth.scalar;
    RCPoint *initial = [[RCPoint alloc] initWithX:0. withY:0. withZ:median];
    RCPoint *start = [data.transformation.rotation transformPoint:[data.cameraTransformation transformPoint:initial]];
    return start;
}

- (void) sensorFusionDidUpdate:(RCSensorFusionData*)data
{
    double currentTime = CACurrentMediaTime();
    double time_in_state = currentTime - lastTransitionTime;
    [self updateProgress:data.status.calibrationProgress];
    if(data.status.calibrationProgress >= 1.) {
        [self handleStateEvent:EV_CONVERGED];
    }
    if(data.status.isSteady && time_in_state > stateTimeout) [self handleStateEvent:EV_STEADY_TIMEOUT];
    
    double time_since_fail = currentTime - lastFailTime;
    if(time_since_fail > failTimeout) [self handleStateEvent:EV_FAIL_EXPIRED];

    
    CVImageBufferRef pixelBuffer = CMSampleBufferGetImageBuffer(data.sampleBuffer);
    if([self.arView.videoView beginFrame])
    {
        [self.arView.videoView displayPixelBuffer:pixelBuffer];
        [self.arView.videoView endFrame];
    }
    [self.arView.featuresLayer updateFeatures:data.featurePoints];
    
    sfData = data;
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

- (void)didDismissOptions
{
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        [SESSION_MANAGER startSession];
    });
}

@end
