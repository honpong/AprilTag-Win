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
	
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

- (IBAction)btnBeginMeasuring:(id)sender {
	NSLog(@"begin measuring");
	UIButton* button = (UIButton*)sender;

	
	if([button.currentTitle isEqualToString:@"Begin Measuring"]){
		[button setTitle:@"Stop Measuring" forState:UIControlStateNormal];
		
		self.lblInstructions.hidden = YES;
		self.lblDistance.hidden = NO;
		self.lblDistance.text = @"Distance: 3 inches";
	} else {
		[button setTitle:@"Begin Measuring" forState:UIControlStateNormal];
		
		self.lblDistance.hidden = YES;
		self.lblInstructions.hidden = NO;
	}
}
- (void)viewDidUnload {
	[self setLblDistance:nil];
	[self setLblInstructions:nil];
	[super viewDidUnload];
}
@end
