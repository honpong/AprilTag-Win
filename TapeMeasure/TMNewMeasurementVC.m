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

- (void)updateProgress:(float)progress
{
    if (progress >= 1)
    {
        [hud hide:YES];
        [self measuringFinished];
    }
    else
    {
        hud.progress = progress;
    }
}

void TMNewMeasurementVCUpdateProgress(void *self, float percent)
{
    [(__bridge id)self updateProgress:percent];
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
    
	isMeasuring = NO;
    useLocation = [[NSUserDefaults standardUserDefaults] boolForKey:PREF_ADD_LOCATION] && [CLLocationManager locationServicesEnabled];
		
    NSMutableArray *navigationArray = [[NSMutableArray alloc] initWithArray: self.navigationController.viewControllers];
    
    if(navigationArray.count > 1) 
    {
        [navigationArray removeObjectAtIndex: 1];  // remove Choose Type VC from nav array, so back button goes to history instead
        self.navigationController.viewControllers = navigationArray;
    }
}

- (void)setupVideoPreview
{
    self.videoPreviewView.clipsToBounds = YES;
	
	CGRect videoRect = self.videoPreviewView.bounds;
    
    SESSION_MANAGER.videoPreviewLayer.frame = videoRect;
	SESSION_MANAGER.videoPreviewLayer.videoGravity = AVLayerVideoGravityResizeAspectFill; //fill view, cropping if necessary
	
	[self.videoPreviewView.layer addSublayer:SESSION_MANAGER.videoPreviewLayer];
}

- (void)viewDidUnload
{
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
    [SESSION_MANAGER startSession]; //should already be running if coming from Choose Type screen, but could be paused if resuming after a pause
    [self handleResume];
}

- (void) viewWillDisappear:(BOOL)animated
{
    [self handlePause];
    [self performSelectorInBackground:@selector(endSession) withObject:nil];
}

- (void)endSession
{
    [SESSION_MANAGER endSession];
}

- (void)handlePause
{
	NSLog(@"handlePause");
	
	[self stopMeasuring];
}

- (void)handleResume
{
	NSLog(@"handleResume");
	
	//watch inertial sensors on background thread
//	[self performSelectorInBackground:(@selector(watchDeviceMotion)) withObject:nil];
    [self performSelectorInBackground:@selector(setupVideoPreview) withObject:nil]; //background thread helps UI load faster
    [self prepareForMeasuring];
}

//handles button tap event
- (IBAction)handleButtonTap:(id)sender
{
	[self toggleMeasuring];
}

- (void)prepareForMeasuring
{
    NSLog(@"prepareForMeasuring");
    
    [self.btnBegin setTitle:@"Begin Measuring"];
    self.btnBegin.enabled = YES;
    
    self.distanceBg.hidden = YES;
    self.lblDistance.hidden = YES;
    
    self.instructionsBg.hidden = NO;
    self.lblInstructions.hidden = NO;
    
    self.instructionsBg.alpha = 0.3;
    self.lblInstructions.alpha = 1;
    
    self.btnSave.enabled = NO;
    
    [self setLocationButtonState];
    
    [self fadeOut:self.lblInstructions withDuration:2 andWait:5];
    [self fadeOut:self.instructionsBg withDuration:2 andWait:5];
    
    //make sure we have up to date location data
    [LOCATION_MANAGER startLocationUpdates];
    
    newMeasurement = [TMMeasurement getNewMeasurement];
}

- (void)startMeasuring
{
    NSLog(@"startMeasuring");
    
	if(!isMeasuring)
	{
		[Flurry
         logEvent:@"Measurement.Start"
         withParameters:[NSDictionary dictionaryWithObjectsAndKeys:useLocation ? @"Yes" : @"No", @"WithLocation", nil]
         ];
        
        [self.btnBegin setTitle:@"Stop Measuring"];
		
		self.lblInstructions.hidden = YES;
        self.instructionsBg.hidden = YES;
        
        self.distanceBg.hidden = NO;
		self.lblDistance.hidden = NO;
		self.lblDistance.text = [NSString stringWithFormat:@"Distance: %@", [newMeasurement getFormattedDistance:0]];
        
        self.btnSave.enabled = NO;
        self.btnPageCurl.enabled = NO;
        		
		distanceMeasured = 0;

        if(CAPTURE_DATA)
        {
            [CORVIS_MANAGER setupPluginsWithFilter:false withCapture:true withReplay:false withUpdateProgress:TMNewMeasurementVCUpdateProgress withUpdateMeasurement:TMNewMeasurementVCUpdateMeasurement withCallbackObject:(__bridge void *)self];
            [CORVIS_MANAGER startPlugins];
            [MOTIONCAP_MANAGER startMotionCap];
            [VIDEOCAP_MANAGER startVideoCap];
		}
        
		isMeasuring = YES;
	}
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
}

- (void)stopMeasuring
{
    NSLog(@"stopMeasuring");
    
	if(isMeasuring)
	{
		[Flurry logEvent:@"Measurement.Stop"];
        
        self.btnBegin.enabled = NO;

        if(CAPTURE_DATA)
        {
            [self startProcessingMeasurement];
        }
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
    
    NSLog(@"shutdownDataCapture:end");
}

- (void)measuringFinished
{
    isMeasuring = NO;
    
//    [CORVIS_MANAGER stopPlugins];
//    [CORVIS_MANAGER teardownPlugins];
    
    self.btnSave.enabled = YES;
    self.btnPageCurl.enabled = YES;
    
    [self.btnBegin setTitle:@"Begin Measuring"];
    self.btnBegin.enabled = YES;
}

- (void)startProcessingMeasurement
{
    hud = [[MBProgressHUD alloc] initWithView:self.navigationController.view];
    hud.mode = MBProgressHUDModeAnnularDeterminate;
    hud.labelText = @"Thinking";
    [self.navigationController.view addSubview:hud];
    [hud show:YES];
    
    [self processMeasurementInBackground];
}

- (void)processMeasurementInBackground
{
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^
    {
        [self shutdownDataCapture];
        
//        [CORVIS_MANAGER setupPluginsWithFilter:true withCapture:false withReplay:true withUpdateProgress:TMNewMeasurementVCUpdateProgress withUpdateMeasurement:TMNewMeasurementVCUpdateMeasurement withCallbackObject:(__bridge void *)self];
//        [CORVIS_MANAGER startPlugins];
        
        //fake progress
        float progress = 0;
        while (progress < 1)
        {
            progress += 0.01f;
            
            dispatch_async(dispatch_get_main_queue(), ^{
                [self updateProgress:progress];
            });
            
            [NSThread sleepForTimeInterval:0.05];
        }
    });
}

- (void)toggleMeasuring
{
	if(!isMeasuring){
        [self startMeasuring];
	} else {
        [self stopMeasuring];
	}
}

- (void)saveMeasurement
{
    NSLog(@"saveMeasurement");
    
    newMeasurement.type = self.type;
    newMeasurement.totalPath = newMeasurement.pointToPoint;
    newMeasurement.horzDist = newMeasurement.pointToPoint;
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
    
    [Flurry logEvent:@"Measurement.Save"];
    
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

-(void)fadeIn:(UIView*)viewToFade withDuration:(NSTimeInterval)duration  withAlpha:(float)alpha andWait:(NSTimeInterval)wait
{
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

- (IBAction)startRepeatingTimer:sender
{
	//cancel any preexisting timer
	[repeatingTimer invalidate];
	
//	progress = 0;
    
    NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:0.1
													target:self
													selector:@selector(targetMethod:)
													userInfo:nil
													repeats:YES];
	
	//store reference to timer object, so we can stop it later
    repeatingTimer = timer;
}

- (void)updateDistanceLabel
{
    NSString *distString = [newMeasurement getFormattedDistance:newMeasurement.pointToPoint];
	self.lblDistance.text = [NSString stringWithFormat:@"Distance: %@", distString];
}

//this method is called by the timer object every tick
- (void)targetMethod:(NSTimer*)theTimer
{
//    newMeasurement.pointToPoint = newMeasurement.pointToPoint + 0.01f;
//    [newMeasurement autoSelectUnitsScale];
//    
//    [self updateDistanceLabel];
    
//    progress += 0.01f;
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
    [Flurry logEvent:@"Measurement.ViewOptions.NewMeasurement"];
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
        TMOptionsVC *optionsVC = [segue destinationViewController];
        optionsVC.theMeasurement = newMeasurement;
        
        [[segue destinationViewController] setDelegate:self];
    }
}

- (void)didDismissOptions
{
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
    }
}

@end
