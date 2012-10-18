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
	
	motionMan = [[CMMotionManager alloc] init];
	motionMan.accelerometerUpdateInterval = 1.0/60;
	[motionMan startAccelerometerUpdates];
	
	[self performSelectorInBackground:(@selector(watchDeviceMotion)) withObject:nil];
//	[self watchDeviceMotion];
}

- (void)viewDidUnload
{
	[motionMan stopDeviceMotionUpdates];
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

//handles button tap event
- (IBAction)handleButtonTap:(id)sender
{
	[self toggleMeasuring];
}

- (void)toggleMeasuring
{
	if(!isMeasuring){
		//start measuring
		NSLog(@"begin measuring");
		
		[self.btnBegin setTitle:@"Stop Measuring" forState:UIControlStateNormal];
		
		self.lblInstructions.hidden = YES;
		self.lblDistance.hidden = NO;
		self.lblDistance.text = @"Distance: 0 inches";
		
		//turn button red
		[self.btnBegin setBackgroundImage:[UIImage imageNamed:@"red_button_bg.png"] forState:UIControlStateNormal];
		
		distanceMeasured = 0;
		[self startRepeatingTimer:nil]; //starts timer that increments distance measured every second
		
		isMeasuring = YES;
	} else {
		//stop measuring
		NSLog(@"stop measuring");
		
		[self.btnBegin setTitle:@"Begin Measuring" forState:UIControlStateNormal];
		[self.btnBegin setBackgroundImage:[UIImage imageNamed:@"green_button_bg.png"] forState:UIControlStateNormal];
		
		[repeatingTimer invalidate]; //stop timer
		
		isMeasuring = NO;
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

- (void) watchDeviceMotion
{
	NSLog(@"Watching device motion");
		
	while (motionMan.isAccelerometerActive) { //we stop accelerometer updates when the view unloads, which will stop this thread.
//		NSLog(@"y: %f", motionMan.accelerometerData.acceleration.y);
		float absAccel = fabs(motionMan.accelerometerData.acceleration.y);
		if(!lastAccel) lastAccel = absAccel; //if lastAccel has not been set, make it equal to current accel
		float accelChange = absAccel - lastAccel;
		lastAccel = absAccel;
		
		if(accelChange > 0.3f) { //change this value to adjust sensitivity
			[self performSelectorOnMainThread:(@selector(handleBump)) withObject:nil waitUntilDone:YES];
		}
		
		[NSThread sleepForTimeInterval: 1.0/60];
	}
	
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
