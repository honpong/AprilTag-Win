//
//  TMOptionsVCViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMOptionsVC.h"
#import "TMResultsVC.h"
#import "TMMeasurement.h"

@interface TMOptionsVC ()

@end

@implementation TMOptionsVC
@synthesize delegate, theMeasurement, btnFractional, btnScale, btnUnits;

- (void)setButtonStates
{
    [self.btnUnits setSelectedSegmentIndex:theMeasurement.units.integerValue];
    [self.btnFractional setSelectedSegmentIndex:theMeasurement.fractional.integerValue];
    
    if(theMeasurement.units.integerValue == UNITS_PREF_METRIC)
    {
        [self.btnScale setSelectedSegmentIndex:theMeasurement.unitsScaleMetric.integerValue];
    }
    else
    {
        [self.btnScale setSelectedSegmentIndex:theMeasurement.unitsScaleImperial.integerValue];
    }
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	self.view.backgroundColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"background_linen.png"]];
    
    [self setScaleButtons];
    [self setButtonStates];
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
    if(theMeasurement.units.integerValue == UNITS_PREF_METRIC)
    {
        [self.btnScale removeAllSegments];
        
        [self.btnScale insertSegmentWithTitle:@"km" atIndex:self.btnScale.numberOfSegments animated:YES];
        [self.btnScale insertSegmentWithTitle:@"m" atIndex:self.btnScale.numberOfSegments animated:YES];
        [self.btnScale insertSegmentWithTitle:@"cm" atIndex:self.btnScale.numberOfSegments animated:YES];
        
        self.btnScale.selectedSegmentIndex = theMeasurement.unitsScaleMetric.integerValue;
        
        self.btnFractional.selectedSegmentIndex = FRACTIONAL_PREF_NO; //decimal
        self.btnFractional.enabled = NO;
    }
    else
    {
        [self.btnScale removeAllSegments];
        
        [self.btnScale insertSegmentWithTitle:@"mi" atIndex:self.btnScale.numberOfSegments animated:YES];
        [self.btnScale insertSegmentWithTitle:@"yd" atIndex:self.btnScale.numberOfSegments animated:YES];
        [self.btnScale insertSegmentWithTitle:@"ft" atIndex:self.btnScale.numberOfSegments animated:YES];
        [self.btnScale insertSegmentWithTitle:@"in" atIndex:self.btnScale.numberOfSegments animated:YES];
        
        self.btnScale.selectedSegmentIndex = theMeasurement.unitsScaleImperial.integerValue;
        
        self.btnFractional.enabled = YES;
        self.btnFractional.selectedSegmentIndex = theMeasurement.fractional.integerValue;
    }
}

- (IBAction)handleUnitsButton:(id)sender {
    theMeasurement.units = [NSNumber numberWithInteger:btnUnits.selectedSegmentIndex];
    
    [self setScaleButtons];    
}

- (IBAction)handleScaleButton:(id)sender {
    if(theMeasurement.units.integerValue == UNITS_PREF_METRIC)
    {
        theMeasurement.unitsScaleMetric = [NSNumber numberWithInteger:self.btnScale.selectedSegmentIndex];
    }
    else
    {
        theMeasurement.unitsScaleImperial = [NSNumber numberWithInteger:self.btnScale.selectedSegmentIndex];
    }
}

- (IBAction)handleFractionalButton:(id)sender {
    theMeasurement.fractional = [NSNumber numberWithInteger:self.btnFractional.selectedSegmentIndex];
}

- (void)saveMeasurement
{
    NSError *error;
    [theMeasurement.managedObjectContext save:&error]; //TODO: Handle save error
    if(error) NSLog(@"Error in saveMeasurement");
}
@end
