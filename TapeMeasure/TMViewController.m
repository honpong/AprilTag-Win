//
//  TMViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 10/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMViewController.h"

@interface TMViewController ()

@end

@implementation TMViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
	isMeasuring = NO;
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

//handles button tap event
- (IBAction)btnBeginMeasuring:(id)sender {
	NSLog(@"begin measuring");
	UIButton* button = (UIButton*)sender;

	
	if(!isMeasuring){
		//start measuring
		[button setTitle:@"Stop Measuring" forState:UIControlStateNormal];
		
		self.lblInstructions.hidden = YES;
		self.lblDistance.hidden = NO;
		self.lblDistance.text = @"Distance: 0 inches";
		
		distanceMeasured = 0;
		[self startRepeatingTimer:nil]; //starts timer that increments distance measured every second
		
		isMeasuring = YES;
	} else {
		//stop measuring
		[button setTitle:@"Begin Measuring" forState:UIControlStateNormal];
		
		[repeatingTimer invalidate]; //stop timer
		
		isMeasuring = NO;
	}
}

- (IBAction)startRepeatingTimer:sender {
	//cancel any preexisting timer
	[repeatingTimer invalidate];
		
    NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:1.0
													target:self
													selector:@selector(targetMethod:)
													userInfo:nil
													repeats:YES];
	
	//store reference to timer object, so we can stop it later
    repeatingTimer = timer;
}

//this method is called by the timer object every tick
- (void)targetMethod:(NSTimer*)theTimer {
	distanceMeasured++;
	self.lblDistance.text = [NSString stringWithFormat:@"Distance: %i inches", distanceMeasured];
}

- (void)viewDidUnload {
	
	[repeatingTimer invalidate];
	[self setLblDistance:nil];
	[self setLblInstructions:nil];
	[super viewDidUnload];
}
@end
