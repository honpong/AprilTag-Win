//
//  TMViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMNewMeasurementVC.h"

@interface TMNewMeasurementVC ()

@end

@implementation TMNewMeasurementVC

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
    self.isProcessingData = NO;
    self.isMeasurementComplete = NO;
    self.isMeasurementCanceled = NO;
    
    useLocation = [LOCATION_MANAGER isLocationAuthorized] && [[NSUserDefaults standardUserDefaults] boolForKey:PREF_ADD_LOCATION];
	
    NSMutableArray *navigationArray = [[NSMutableArray alloc] initWithArray: self.navigationController.viewControllers];
    
    if(navigationArray.count > 1) 
    {
        [navigationArray removeObjectAtIndex: 1];  // remove Choose Type VC from nav array, so back button goes to history instead
        self.navigationController.viewControllers = navigationArray;
    }
    
    self.distanceBg.hidden = YES;
    self.lblDistance.hidden = YES;
    
    self.instructionsBg.hidden = NO;
    self.lblInstructions.hidden = NO;
    
    self.instructionsBg.alpha = 0.3;
    self.lblInstructions.alpha = 1;
    
    [self setLocationButtonState];
    
    [self fadeOut:self.lblInstructions withDuration:2 andWait:5];
    [self fadeOut:self.instructionsBg withDuration:2 andWait:5];
}

- (void)viewDidUnload
{
	NSLog(@"viewDidUnload");
    [repeatingTimer invalidate];
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
    [self handleResume];
}

- (void) viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    [self handlePause];
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
    [CORVIS_MANAGER teardownPlugins];
}

- (void)handleResume
{
	NSLog(@"handleResume");
	
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
}

- (void)setupVideoPreview
{
    NSLog(@"setupVideoPreview");

    self.videoPreviewView.clipsToBounds = YES;
    SESSION_MANAGER.videoPreviewLayer.videoGravity = AVLayerVideoGravityResizeAspect; //fill view, cropping if necessary
    
    [self setupVideoPreviewFrame];
    [self.videoPreviewView.layer addSublayer:SESSION_MANAGER.videoPreviewLayer];
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
    
    //make sure we have up to date location data
    if (useLocation) [LOCATION_MANAGER startLocationUpdates];
    
    newMeasurement = [TMMeasurement getNewMeasurement];
    
    self.isMeasurementCanceled = NO;
    
    if(CAPTURE_DATA)
    {
        CLLocation *loc = [LOCATION_MANAGER getStoredLocation];
        
        [CORVIS_MANAGER
         setupPluginsWithFilter:true
         withCapture:false
         withReplay:false
         withLocationValid:loc ? true : false
         withLatitude:loc ? loc.coordinate.latitude : 0
         withLongitude:loc ? loc.coordinate.longitude : 0
         withAltitude:loc ? loc.altitude : 0
         withUpdateProgress:TMNewMeasurementVCUpdateProgress
         withUpdateMeasurement:TMNewMeasurementVCUpdateMeasurement
         withCallbackObject:(__bridge void *)(self)];
        [CORVIS_MANAGER startPlugins];
        [MOTIONCAP_MANAGER startMotionCap];
        [VIDEOCAP_MANAGER startVideoCap];
        self.isCapturingData = YES;
    }
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
    
    self.lblInstructions.hidden = YES;
    self.instructionsBg.hidden = YES;
    
    self.distanceBg.hidden = NO;
    self.lblDistance.hidden = NO;
    
    self.btnSave.enabled = NO;
    self.btnPageCurl.enabled = NO;
            
    distanceMeasured = 0;

    if(CAPTURE_DATA)
    {
        [CORVIS_MANAGER startMeasurement];
    }
    self.isMeasuring = YES;
}

- (void)updateMeasurementDataWithX:(float)x stdx:(float)stdx y:(float)y stdy:(float)stdy z:(float)z stdz:(float)stdz path:(float)path stdpath:(float)stdpath
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

    dispatch_async(dispatch_get_main_queue(), ^{
        [newMeasurement autoSelectUnitsScale];
        self.lblDistance.text = [NSString stringWithFormat:@"%@", [newMeasurement getFormattedDistance:[newMeasurement getPrimaryMeasurementDist]]];
    });
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
    
    hud = [[MBProgressHUD alloc] initWithView:self.navigationController.view];
    hud.mode = MBProgressHUDModeAnnularDeterminate;
    hud.labelText = @"Thinking";
    [self.navigationController.view addSubview:hud];
    [hud show:YES];
    
    self.navigationItem.hidesBackButton = YES;
    self.btnBegin.enabled = NO;
    self.locationButton.enabled = NO;

    if(CAPTURE_DATA)
    {
        [self shutdownDataCapture];
        [self processingFinished];
    }
    self.isMeasuring = NO;
}

- (void)cancelMeasuring
{
    NSLog(@"cancelMeasuring");
        
    [hud hide:YES];
    
    if (self.isCapturingData)
    {
        [self shutdownDataCapture];
        self.isMeasurementCanceled = YES;
    }
    else if (self.isProcessingData)
    {
        self.isProcessingData = NO;
        self.isMeasurementCanceled = YES;
    }
}

- (void)shutdownDataCapture
{
    NSLog(@"shutdownDataCapture:begin");
    
    [VIDEOCAP_MANAGER stopVideoCap];
    [MOTIONCAP_MANAGER stopMotionCap];
    
    [NSThread sleepForTimeInterval:0.2]; //hack to prevent CorvisManager from receiving a video frame after plugins have stopped.
    
    [CORVIS_MANAGER stopPlugins];
    
    self.isCapturingData = NO;
    
    NSLog(@"shutdownDataCapture:end");
}

- (void)processingFinished
{
    NSLog(@"processingFinished");
    
    self.isProcessingData = NO;
    self.isMeasurementComplete = YES;
    
    [hud hide:YES];
    
    //don't need to call stopPlugins
//    [CORVIS_MANAGER teardownPlugins];
    
    self.navigationItem.hidesBackButton = NO;
    self.btnSave.enabled = YES;
    self.btnPageCurl.enabled = YES;
    self.locationButton.enabled = YES;
}

- (void)processMeasurement
{
    NSLog(@"processMeasurement");
    
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^
    {
        [self shutdownDataCapture];
        
        [CORVIS_MANAGER teardownPlugins];
        CLLocation *loc = [LOCATION_MANAGER getStoredLocation];
        [CORVIS_MANAGER
         setupPluginsWithFilter:true
         withCapture:false withReplay:true
         withLocationValid:loc ? true : false
         withLatitude:loc ? loc.coordinate.latitude : 0
         withLongitude:loc ? loc.coordinate.longitude : 0
         withAltitude:loc ? loc.altitude : 0
         withUpdateProgress:TMNewMeasurementVCUpdateProgress
         withUpdateMeasurement:TMNewMeasurementVCUpdateMeasurement
         withCallbackObject:(__bridge void *)self];
        
        if (!self.isMeasurementCanceled)
        {
            [CORVIS_MANAGER startPlugins];
            self.isProcessingData = YES;
        }
        
        //fake progress
        /*float progress = 0;
        while (progress < 1)
        {
            progress += 0.01f;
            
            dispatch_async(dispatch_get_main_queue(), ^{
                [self updateProgress:progress];
            });
            
            [NSThread sleepForTimeInterval:0.05];
        }*/
    });
}

- (void)updateProgress:(float)progress
{
    if (progress >= 1)
    {
        [self processingFinished];
    }
    else
    {
        hud.progress = progress;
    }
}

void TMNewMeasurementVCUpdateProgress(void *self, float percent)
{
//    [(__bridge id)self updateProgress:percent];
}

void TMNewMeasurementVCUpdateMeasurement(void *self, float x, float stdx, float y, float stdy, float z, float stdz, float path, float stdpath)
{
    [(__bridge id)self updateMeasurementDataWithX:(float)x
                                             stdx:(float)stdx
                                                y:(float)y
                                             stdy:(float)stdy
                                                z:(float)z
                                             stdz:(float)stdz
                                             path:(float)path
                                          stdpath:(float)stdpath];
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
