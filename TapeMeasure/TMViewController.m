//
//  TMViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMViewController.h"
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

@interface TMViewController ()

@end

@implementation TMViewController
@synthesize context = _context;

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
	
	[self setupMotionCapture];
	[self setupVideoCapture];
}

- (void)setupMotionCapture
{
	motionMan = [[CMMotionManager alloc] init];
	motionCap = [[RCMotionCap alloc] initWithMotionManager:motionMan withOutput:&_databuffer];
}

- (void)setupVideoCapture
{
	NSError * error;
	avSession = [[AVCaptureSession alloc] init];
	
    [avSession beginConfiguration];
    [avSession setSessionPreset:AVCaptureSessionPreset640x480];
	
    AVCaptureDevice * videoDevice = [AVCaptureDevice defaultDeviceWithMediaType:AVMediaTypeVideo];
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

        [motionCap startMotionCapture];
        [videoCap startVideoCap];
		
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

        [videoCap stopVideoCap];
        [motionCap stopMotionCapture];

        plugins_stop();
        
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

}

@end
