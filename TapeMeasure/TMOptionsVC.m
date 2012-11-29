//
//  TMOptionsVCViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMOptionsVC.h"
#import "TMHistoryVC.h"

@interface TMOptionsVC ()

@end

@implementation TMOptionsVC
@synthesize delegate;

- (void)viewDidLoad
{
    [super viewDidLoad];
	self.view.backgroundColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"background_linen.png"]];
    
    int unitsPref = [[NSUserDefaults standardUserDefaults] integerForKey:@"Units"];
    int fractionalPref = [[NSUserDefaults standardUserDefaults] integerForKey:@"Fractional"];
    
    [self.btnUnits setSelectedSegmentIndex:unitsPref];
    [self.btnFractional setSelectedSegmentIndex:fractionalPref];
}

- (void)viewWillDisappear:(BOOL)animated
{
    NSLog(@"viewWillDisappear");
    [delegate didDismissModalView];
}

- (void)viewDidUnload {
    [self setBtnFractional:nil];
    [self setBtnUnits:nil];
    [super viewDidUnload];
}

- (IBAction)handleUnitsButton:(id)sender {
    [[NSUserDefaults standardUserDefaults] setInteger:self.btnUnits.selectedSegmentIndex forKey:@"Units"];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

- (IBAction)handleFractionalButton:(id)sender {
    [[NSUserDefaults standardUserDefaults] setInteger:self.btnFractional.selectedSegmentIndex forKey:@"Fractional"];
    [[NSUserDefaults standardUserDefaults] synchronize];
}
@end
