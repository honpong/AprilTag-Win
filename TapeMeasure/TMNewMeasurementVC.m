//
//  TMViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMNewMeasurementVC.h"
#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>
#import <AVFoundation/AVFoundation.h>
#import <CoreMedia/CoreMedia.h>
#import <CoreVideo/CoreVideo.h>
#import <QuartzCore/QuartzCore.h>
#import <CoreImage/CoreImage.h>
#import <ImageIO/ImageIO.h>
#import <CoreGraphics/CoreGraphics.h>
#import "RCCore/RCMotionCap.h"
#import "RCCore/cor.h"
#import "TMAppDelegate.h"
#import "TMMeasurement.h"
#import "TMResultsVC.h"
#import "TMDistanceFormatter.h"
#import "TMOptionsVC.h"
#import "TMLocation.h"
#import <CoreLocation/CoreLocation.h>

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
    
    TMAppDelegate* appDel = (TMAppDelegate*)[[UIApplication sharedApplication] delegate];
    _managedObjectContext = appDel.managedObjectContext;
    
	isMeasuring = NO;
    useLocation = YES; //TODO: make this a global pref
		
    [self performSelectorInBackground:@selector(setupVideoCapture) withObject:nil]; //background thread helps UI load faster
    [self performSelectorInBackground:@selector(setupMotionCapture) withObject:nil];
	
    NSMutableArray *navigationArray = [[NSMutableArray alloc] initWithArray: self.navigationController.viewControllers];
    
    if(navigationArray.count > 1) 
    {
        [navigationArray removeObjectAtIndex: 1];  // remove Choose Type VC from nav array, so back button goes to history instead
        self.navigationController.viewControllers = navigationArray;
    }
}

-(void) viewDidAppear:(BOOL)animated
{
    [self prepareForMeasuring];
}

- (void)setupMotionCapture
{
	motionMan = [[CMMotionManager alloc] init];
	motionCap = [[RCMotionCap alloc] initWithMotionManager:motionMan withOutput:&_databuffer];
}

- (AVCaptureDevice *) cameraWithPosition:(AVCaptureDevicePosition) position
{
    NSArray *devices = [AVCaptureDevice devicesWithMediaType:AVMediaTypeVideo];
    for (AVCaptureDevice *device in devices) {
        if ([device position] == position) {
            return device;
        }
    }
    return nil;
}

- (void)setupVideoCapture
{
	NSError * error;
	avSession = [[AVCaptureSession alloc] init];
	
    [avSession beginConfiguration];
    [avSession setSessionPreset:AVCaptureSessionPreset640x480];
	
    AVCaptureDevice * videoDevice = [self cameraWithPosition:AVCaptureDevicePositionFront];
    /*[AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];

    // SETUP FOCUS MODE
    if ([videoDevice lockForConfiguration:nil]) {
        [videoDevice setFocusMode:AVCaptureFocusModeLocked];
        NSLog(@"Focus mode locked");
    }
    else{
        NSLog(@"error while configuring focusMode");
    }*/
    
    AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:videoDevice error:&error];
    
    [avSession addInput:input];
	[avSession commitConfiguration];
    [avSession startRunning];
    
    videoCap = [[RCVideoCap alloc] initWithSession:avSession withOutput:&_databuffer];
	
    [self setupVideoPreview];
    
//	//setup context for image processing
//	self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
//	[EAGLContext setCurrentContext:self.context];
//	
//    if (!self.context) {
//        NSLog(@"Failed to create EAGLContext");
//    }
//	
////	self.videoPreviewView.context = self.context;
//	
//	CAEAGLLayer *eaglLayer = (CAEAGLLayer*)self.videoPreviewView.layer;
//	[eaglLayer setOpaque: YES];
//	[eaglLayer setFrame: self.videoPreviewView.bounds];
//	
//	glGenRenderbuffers(1, &_renderBuffer); 
//    glBindRenderbuffer(GL_RENDERBUFFER, _renderBuffer);
//	
//	NSMutableDictionary *options = [[NSMutableDictionary alloc] init];
//    [options setObject: [NSNull null] forKey: kCIContextWorkingColorSpace]; //disable color management
//	
//    ciContext = [CIContext contextWithEAGLContext:self.context options:options];

}

- (void)setupVideoPreview
{
	AVCaptureVideoPreviewLayer *captureVideoPreviewLayer = [[AVCaptureVideoPreviewLayer alloc] initWithSession:avSession];
	
	self.videoPreviewView.clipsToBounds = YES;
	
	CGRect videoRect = self.videoPreviewView.bounds;	
	captureVideoPreviewLayer.frame = videoRect;
	captureVideoPreviewLayer.videoGravity = AVLayerVideoGravityResizeAspectFill; //fill view, cropping if necessary
	
	[self.videoPreviewView.layer addSublayer:captureVideoPreviewLayer];
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
    TMAppDelegate* appDel = (TMAppDelegate*)[[UIApplication sharedApplication] delegate];
    [appDel startLocationUpdates];
    
    //here, we create the new instance of our model object, but do not yet insert it into the persistent store
    NSEntityDescription *entity = [NSEntityDescription entityForName:ENTITY_MEASUREMENT inManagedObjectContext:_managedObjectContext];
    newMeasurement = (TMMeasurement*)[[NSManagedObject alloc] initWithEntity:entity insertIntoManagedObjectContext:nil];
}

- (void)setupCorStuff
{
    NSArray  *documentDirList = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentDir  = [documentDirList objectAtIndex:0];
    NSString *documentPath = [documentDir stringByAppendingPathComponent:@"latest"];
    const char *filename = [documentPath cStringUsingEncoding:NSUTF8StringEncoding];
    _databuffer.filename = filename;
    _databuffer.size = .5 * 1024 * 1024 * 1024;
    _databuffer.indexsize = 600000;
    _databuffer.ahead = 64 * 1024 * 1024;
    _databuffer.behind = 16 * 1024 * 1024;
    _databuffer.blocksize = 256 * 1024;
    _databuffer.mem_writable = true;
    _databuffer.file_writable = true;
    _databuffer.threaded = true;
    struct plugin mbp = mapbuffer_open(&_databuffer);
    plugins_register(mbp);
    
    cor_time_init();
    
    plugins_start();
}

- (void)beginMeasuring
{
    NSLog(@"beginMeasuring");
    
	if(!isMeasuring)
	{
		[self.btnBegin setTitle:@"Stop Measuring"];
		
		self.lblInstructions.hidden = YES;
        self.instructionsBg.hidden = YES;
        
        self.distanceBg.hidden = NO;
		self.lblDistance.hidden = NO;
		self.lblDistance.text = @"Distance: 0 inches";
        
        self.btnSave.enabled = NO;
		
		distanceMeasured = 0;
		[self startRepeatingTimer:nil]; //starts timer that increments distance measured every second

        if(CAPTURE_DATA)
        {
            [self setupCorStuff];
            [motionCap startMotionCapture];
            [videoCap startVideoCap];
		}
        
		isMeasuring = YES;
	}
}

- (void)stopMeasuring
{
    NSLog(@"stopMeasuring");
    
	if(isMeasuring)
	{
		[self.btnBegin setTitle:@"Begin Measuring"];
		
		[repeatingTimer invalidate]; //stop timer

        if(CAPTURE_DATA)
        {
            [videoCap stopVideoCap];
            [motionCap stopMotionCapture];
            plugins_stop();
        }
        
		isMeasuring = NO;
        
        self.btnSave.enabled = YES;
    }
}

- (void)toggleMeasuring
{
	if(!isMeasuring){
        [self beginMeasuring];
	} else {
        [self stopMeasuring];
	}
}

- (void)saveMeasurement
{
    NSLog(@"saveMeasurement");
    
    newMeasurement.totalPath = newMeasurement.pointToPoint;
    newMeasurement.horzDist = newMeasurement.pointToPoint;
    newMeasurement.timestamp = [NSDate dateWithTimeIntervalSinceNow:0];
    
    [_managedObjectContext insertObject:newMeasurement]; //order is important. this must be inserted before location is added.
    
    TMAppDelegate* appDel = (TMAppDelegate*)[[UIApplication sharedApplication] delegate];
    
    //add location to measurement
    if(useLocation && appDel.locationAddress)
    {
        NSEntityDescription *entity = [NSEntityDescription entityForName:ENTITY_LOCATION inManagedObjectContext:_managedObjectContext];
        
        TMLocation *location = (TMLocation*)[[NSManagedObject alloc] initWithEntity:entity insertIntoManagedObjectContext:_managedObjectContext];
        location.latititude = [NSNumber numberWithDouble: appDel.location.coordinate.latitude];
        location.longitude = [NSNumber numberWithDouble: appDel.location.coordinate.longitude];
        location.accuracyInMeters = [NSNumber numberWithDouble: appDel.location.horizontalAccuracy];
        
        if(appDel.locationAddress) location.address = appDel.locationAddress;
        
        [location addMeasurementObject:newMeasurement];
    }
    
    NSError *error;
    [_managedObjectContext save:&error];
    
//    [self postMeasurement];
}

-(void)postMeasurement
{
    NSString *bodyData = [NSString stringWithFormat:@"measurement[user_id]=1&measurement[name]=%@&measurement[value]=%@&measurement[location_id]=3", [self urlEncodeString:newMeasurement.name], newMeasurement.pointToPoint];
    
    NSMutableURLRequest *postRequest = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:@"http://192.168.1.1:3000/measurements.json"]];
    
    [postRequest setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
    [postRequest setHTTPMethod:@"POST"];
    [postRequest setHTTPBody:[NSData dataWithBytes:[bodyData UTF8String] length:[bodyData length]]];
    
    NSURLConnection *theConnection=[[NSURLConnection alloc] initWithRequest:postRequest delegate:self];
    
    if (theConnection)
    {
        NSLog(@"Connection ok");
//        receivedData = [[NSMutableData data] retain];
    }
    else
    {
        NSLog(@"Connection failed");
    }
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
    
    // This method is called when the server has determined that it
    // has enough information to create the NSURLResponse.
        
    // It can be called multiple times, for example in the case of a
    // redirect, so each time we reset the data.
        
    // receivedData is an instance variable declared elsewhere.
    
//    [receivedData setLength:0];
    
    NSLog(@"didReceiveResponse");
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data

{
    // Append the new data to receivedData.
    // receivedData is an instance variable declared elsewhere.
  
//    [receivedData appendData:data];
    
    NSLog(@"didReceiveData");
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    NSLog(@"Connection failed! Error - %@ %@",
        [error localizedDescription],
        [[error userInfo] objectForKey:NSURLErrorFailingURLStringErrorKey]);
}

-(NSString*)urlEncodeString:(NSString*)input
{
    NSString *urlEncoded = (__bridge_transfer NSString *)CFURLCreateStringByAddingPercentEscapes(
                                                                                                 NULL,
                                                                                                 (__bridge CFStringRef)input,
                                                                                                 NULL, 
                                                                                                 (CFStringRef)@"!*'\"();:@&=+$,/?%#[]% ", 
                                                                                                 kCFStringEncodingUTF8);
    return urlEncoded;
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
		
    NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:0.05
													target:self
													selector:@selector(targetMethod:)
													userInfo:nil
													repeats:YES];
	
	//store reference to timer object, so we can stop it later
    repeatingTimer = timer;
}

- (void)updateDistanceLabel
{
    NSString *distString = [TMDistanceFormatter formattedDistance:newMeasurement.pointToPoint withMeasurement:newMeasurement];
	self.lblDistance.text = [NSString stringWithFormat:@"Distance: %@", distString];
}

//this method is called by the timer object every tick
- (void)targetMethod:(NSTimer*)theTimer
{
//	distanceMeasured = distanceMeasured + 0.01f;
    newMeasurement.pointToPoint = [NSNumber numberWithFloat:newMeasurement.pointToPoint.floatValue + 0.01f];
    [self updateDistanceLabel];
}

//this routine is run in a background thread
- (void) watchDeviceMotion
{
	NSLog(@"Watching device motion");
	
	//create log file and write column names as first line
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory,NSUserDomainMask, YES);
    NSString *documentsDirectory = [paths objectAtIndex:0];
    NSString *logFilePath = [documentsDirectory stringByAppendingPathComponent:@"log.txt"];
    
	NSString *colNames = @"timestamp,sensor,x,y,z\n";
	bool isFileCreated = [colNames writeToFile:logFilePath atomically:YES encoding:NSUTF8StringEncoding error:nil];
	if(!isFileCreated) NSLog(@"Failed to create log file");
	
    NSFileHandle *myHandle = [NSFileHandle fileHandleForWritingAtPath:logFilePath];
	
	NSString *logLine;
    		
	while (motionMan.isAccelerometerActive)  //we stop accelerometer updates when the app is paused or terminated, which will stop this thread.
	{
		//detect bump
		float absAccel = fabs(motionMan.accelerometerData.acceleration.y);
		if(!lastAccel) lastAccel = absAccel; //if lastAccel has not been set, make it equal to current accel
		float accelChange = absAccel - lastAccel;
		lastAccel = absAccel;
		
		if(accelChange > 0.3f) { //change this value to adjust bump sensitivity
			[self performSelectorOnMainThread:(@selector(handleBump)) withObject:nil waitUntilDone:YES];
		}
		
		//log inertial data
		if(isMeasuring)
		{
			//append line to log
			logLine = [NSString stringWithFormat:@"%f,accel,%f,%f,%f\n", motionMan.accelerometerData.timestamp, motionMan.accelerometerData.acceleration.x, motionMan.accelerometerData.acceleration.y, motionMan.accelerometerData.acceleration.z];
		
			[myHandle seekToEndOfFile];
			[myHandle writeData:[logLine dataUsingEncoding:NSUTF8StringEncoding]];
			
			//append line to log
			logLine = [NSString stringWithFormat:@"%f,gyro,%f,%f,%f\n", motionMan.gyroData.timestamp, motionMan.gyroData.rotationRate.x, motionMan.gyroData.rotationRate.y, motionMan.gyroData.rotationRate.z];
			
			[myHandle seekToEndOfFile];
			[myHandle writeData:[logLine dataUsingEncoding:NSUTF8StringEncoding]];
		}
		
		[NSThread sleepForTimeInterval: 1.0/60];
	}
	
	[myHandle closeFile];
}

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
//    UIViewController *controller = [self.storyboard instantiateViewControllerWithIdentifier:@"Options"];
//    controller.modalTransitionStyle = UIModalTransitionStylePartialCurl;
//    [self presentModalViewController:controller animated:YES];
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
