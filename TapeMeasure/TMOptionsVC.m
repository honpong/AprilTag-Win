//
//  TMOptionsVCViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMOptionsVC.h"
#import "RCCore/RCDistance.h"

@interface TMOptionsVC ()

@end

@implementation TMOptionsVC
@synthesize delegate, theMeasurement, btnFractional, btnScale, btnUnits;

- (void)setButtonStates
{
    [self.btnUnits setSelectedSegmentIndex:theMeasurement.units];
        
    if(theMeasurement.units == UnitsMetric)
    {
        [self.btnScale setSelectedSegmentIndex:theMeasurement.unitsScaleMetric];
        [self.btnFractional setSelectedSegmentIndex:0];
    }
    else
    {
        [self.btnScale setSelectedSegmentIndex:theMeasurement.unitsScaleImperial];
        [self.btnFractional setSelectedSegmentIndex:theMeasurement.fractional];
    }
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	self.view.backgroundColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"background_linen.png"]];
    
    [self setScaleButtons];
    [self setButtonStates];
}

- (void)viewDidAppear:(BOOL)animated
{
    [TMAnalytics logEvent:@"View.Options"];
    [super viewDidAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [super viewWillDisappear:animated];
    [self saveMeasurement];
    [delegate didDismissOptions];
}

- (void)viewDidUnload {
    [self setBtnFractional:nil];
    [self setBtnUnits:nil];
    [self setBtnScale:nil];
    [super viewDidUnload];
}

- (void)setScaleButtons {
    //switch scale buttons to appropriate type
    if(theMeasurement.units == UnitsMetric)
    {
        [self.btnScale removeAllSegments];
        
        [self.btnScale insertSegmentWithTitle:@"km" atIndex:self.btnScale.numberOfSegments animated:YES];
        [self.btnScale insertSegmentWithTitle:@"m" atIndex:self.btnScale.numberOfSegments animated:YES];
        [self.btnScale insertSegmentWithTitle:@"cm" atIndex:self.btnScale.numberOfSegments animated:YES];
        
        self.btnScale.selectedSegmentIndex = theMeasurement.unitsScaleMetric;
        
        self.btnFractional.selectedSegmentIndex = 0;
        self.btnFractional.enabled = NO;
    }
    else
    {
        [self.btnScale removeAllSegments];
        
        [self.btnScale insertSegmentWithTitle:@"mi" atIndex:self.btnScale.numberOfSegments animated:YES];
        [self.btnScale insertSegmentWithTitle:@"yd" atIndex:self.btnScale.numberOfSegments animated:YES];
        [self.btnScale insertSegmentWithTitle:@"ft" atIndex:self.btnScale.numberOfSegments animated:YES];
        [self.btnScale insertSegmentWithTitle:@"in" atIndex:self.btnScale.numberOfSegments animated:YES];
        
        self.btnScale.selectedSegmentIndex = theMeasurement.unitsScaleImperial;
        
        self.btnFractional.enabled = YES;
        self.btnFractional.selectedSegmentIndex = theMeasurement.fractional;
    }
}

- (IBAction)handleUnitsButton:(id)sender {
    theMeasurement.units = btnUnits.selectedSegmentIndex;
    
    [self setScaleButtons];    
}

- (IBAction)handleScaleButton:(id)sender {
    if(theMeasurement.units == UnitsMetric)
    {
        theMeasurement.unitsScaleMetric = self.btnScale.selectedSegmentIndex;
    }
    else
    {
        theMeasurement.unitsScaleImperial = self.btnScale.selectedSegmentIndex;
    }
}

- (IBAction)handleFractionalButton:(id)sender {
    theMeasurement.fractional = self.btnFractional.selectedSegmentIndex;
}

- (void)saveMeasurement
{
    NSError *error;
    [theMeasurement.managedObjectContext save:&error]; //TODO: Handle save error
    if(error) DLog(@"Error in saveMeasurement");
}
@end
