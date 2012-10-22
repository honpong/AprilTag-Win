//
//  TMViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMViewController.h"
#import <QuartzCore/QuartzCore.h>
#import <CoreMotion/CoreMotion.h>
#import <AVFoundation/AVFoundation.h>

@interface TMViewController ()

@end

@implementation TMViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
	
	self.view.backgroundColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"background_linen.png"]];
	
	isMeasuring = NO;
	
	//give button rounded corners
	CALayer* layer = [self.btnBegin layer];
    [layer setMasksToBounds:YES];
    [layer setCornerRadius:10.0];
    [layer setBorderWidth:1.0];
    [layer setBorderColor:[[UIColor grayColor] CGColor]];
	
	//setup inertial capture
	motionMan = [[CMMotionManager alloc] init];
	motionMan.accelerometerUpdateInterval = 1.0/2;
	motionMan.gyroUpdateInterval = 1.0/60;
	
	NSOperationQueue *queueAccel = [[NSOperationQueue alloc] init];
	[queueAccel setMaxConcurrentOperationCount:1]; //makes this into a serial queue, instead of concurrent
	
	[motionMan startAccelerometerUpdatesToQueue:queueAccel withHandler:
	 ^(CMAccelerometerData *accelerometerData, NSError *error){
		 if (error) {
			 [motionMan stopAccelerometerUpdates];
		 } else {
			 NSString *logLine = [NSString stringWithFormat:@"%f,accel,%f,%f,%f\n", accelerometerData.timestamp, accelerometerData.acceleration.x, accelerometerData.acceleration.y, accelerometerData.acceleration.z];
			 NSLog(logLine);
		 }
	 }];
}

- (void)viewDidUnload
{
	[repeatingTimer invalidate];
	[self setLblDistance:nil];
	[self setLblInstructions:nil];
	[self setBtnBegin:nil];
	[self setVImagePreview:nil];
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
	
	[motionMan stopAccelerometerUpdates]; //has the effect of stopping the bg thread that's watching the inertial sensors
//	[motionMan stopGyroUpdates];
}

- (void)handleResume
{
	NSLog(@"handleResume");
	
//	[motionMan startAccelerometerUpdates];
//	[motionMan startGyroUpdates];
	
	//watch inertial sensors on background thread
	[self performSelectorInBackground:(@selector(watchDeviceMotion)) withObject:nil];
}

//handles button tap event
- (IBAction)handleButtonTap:(id)sender
{
	[self toggleMeasuring];
}

- (void)beginMeasuring
{
    NSLog(@"beginMeasuring");
    
	if(!isMeasuring)
	{
		[self.btnBegin setTitle:@"Stop Measuring" forState:UIControlStateNormal];
		
		self.lblInstructions.hidden = YES;
		self.lblDistance.hidden = NO;
		self.lblDistance.text = @"Distance: 0 inches";
		
		//turn button red
		[self.btnBegin setBackgroundImage:[UIImage imageNamed:@"red_button_bg.png"] forState:UIControlStateNormal];
		
		distanceMeasured = 0;
		[self startRepeatingTimer:nil]; //starts timer that increments distance measured every second
		
		isMeasuring = YES;
	}
}

- (void)stopMeasuring
{
    NSLog(@"stopMeasuring");
    
	if(isMeasuring)
	{
		[self.btnBegin setTitle:@"Begin Measuring" forState:UIControlStateNormal];
		[self.btnBegin setBackgroundImage:[UIImage imageNamed:@"green_button_bg.png"] forState:UIControlStateNormal];
		
		[repeatingTimer invalidate]; //stop timer
		
		isMeasuring = NO;
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

- (IBAction)startRepeatingTimer:sender
{
	//cancel any preexisting timer
	[repeatingTimer invalidate];
		
    NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:0.5
													target:self
													selector:@selector(targetMethod:)
													userInfo:nil
													repeats:YES];
	
	//store reference to timer object, so we can stop it later
    repeatingTimer = timer;
}

//this method is called by the timer object every tick
- (void)targetMethod:(NSTimer*)theTimer
{
	distanceMeasured++;
	self.lblDistance.text = [NSString stringWithFormat:@"Distance: %i inches", distanceMeasured];
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

-(void) viewDidAppear:(BOOL)animated
{
	AVCaptureSession *session = [[AVCaptureSession alloc] init];
	session.sessionPreset = AVCaptureSessionPresetMedium;
	
//	CALayer *viewLayer = self.vImagePreview.layer;
//	NSLog(@"viewLayer = %@", viewLayer);
	
	AVCaptureVideoPreviewLayer *captureVideoPreviewLayer = [[AVCaptureVideoPreviewLayer alloc] initWithSession:session];
	
	self.vImagePreview.clipsToBounds = YES;
	
	CGRect videoRect = self.vImagePreview.bounds;
//	videoRect.size.width = 320;
	videoRect.size.height += videoRect.size.height;
	
	captureVideoPreviewLayer.frame = videoRect;
	captureVideoPreviewLayer.position = CGPointMake(160, 125);
	
	[self.vImagePreview.layer addSublayer:captureVideoPreviewLayer];
	
	AVCaptureDevice *device = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
	
	NSError *error = nil;
	AVCaptureDeviceInput *input = [AVCaptureDeviceInput deviceInputWithDevice:device error:&error];
	if (!input) {
		// Handle the error appropriately.
		NSLog(@"ERROR: trying to open camera: %@", error);
	}
	[session addInput:input];
	
	[session startRunning];
}

@end
