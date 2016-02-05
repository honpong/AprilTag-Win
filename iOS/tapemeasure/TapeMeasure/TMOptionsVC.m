//
//  TMOptionsVCViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMOptionsVC.h"
#import <RCCore/RCCore.h>

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
    
    [self setScaleButtons];
    [self setButtonStates];
    
    if (self.modalPresentationStyle == UIModalPresentationCustom)
    {
        UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapOutside:)];
        tapGesture.numberOfTapsRequired = 1;
        tapGesture.delegate = self;
        [self.view addGestureRecognizer:tapGesture];
    }
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
}

- (void)viewDidUnload {
    [self setBtnFractional:nil];
    [self setBtnUnits:nil];
    [self setBtnScale:nil];
    [super viewDidUnload];
}

- (BOOL)hidesBottomBarWhenPushed { return YES; }

- (void)setScaleButtons {
    //switch scale buttons to appropriate type
    if(theMeasurement.units == UnitsMetric)
    {
        [self.btnScale removeAllSegments];
        
        [self.btnScale insertSegmentWithTitle:@"km" atIndex:self.btnScale.numberOfSegments animated:NO];
        [self.btnScale insertSegmentWithTitle:@"m" atIndex:self.btnScale.numberOfSegments animated:NO];
        [self.btnScale insertSegmentWithTitle:@"cm" atIndex:self.btnScale.numberOfSegments animated:NO];
        
        self.btnScale.selectedSegmentIndex = theMeasurement.unitsScaleMetric;
        
        self.btnFractional.selectedSegmentIndex = 0;
        self.btnFractional.enabled = NO;
    }
    else
    {
        [self.btnScale removeAllSegments];
        
        [self.btnScale insertSegmentWithTitle:@"mi" atIndex:self.btnScale.numberOfSegments animated:NO];
        [self.btnScale insertSegmentWithTitle:@"yd" atIndex:self.btnScale.numberOfSegments animated:NO];
        [self.btnScale insertSegmentWithTitle:@"ft" atIndex:self.btnScale.numberOfSegments animated:NO];
        [self.btnScale insertSegmentWithTitle:@"in" atIndex:self.btnScale.numberOfSegments animated:NO];
        
        self.btnScale.selectedSegmentIndex = theMeasurement.unitsScaleImperial;
        
        self.btnFractional.enabled = YES;
        self.btnFractional.selectedSegmentIndex = theMeasurement.fractional;
    }
    
    [self.containerView setNeedsLayout]; // workaround for bug on iPhone where segmented control doesn't honor it's height constraint.
}

- (IBAction)handleUnitsButton:(id)sender
{
    theMeasurement.units = btnUnits.selectedSegmentIndex;
    
    [self setScaleButtons];
    
    if (btnUnits.selectedSegmentIndex)
        [TMAnalytics logEvent:@"Measurement.Options" withParameters:@{ @"Units" : @"Metric" }];
    else
        [TMAnalytics logEvent:@"Measurement.Options" withParameters:@{ @"Units" : @"Imperial" }];
    
    [delegate didChangeOptions];
}

- (IBAction)handleScaleButton:(id)sender
{
    if(theMeasurement.units == UnitsMetric)
    {
        theMeasurement.unitsScaleMetric = self.btnScale.selectedSegmentIndex;
        
        switch (self.btnScale.selectedSegmentIndex)
        {
            case 0:
            {
                [TMAnalytics logEvent:@"Measurement.Options" withParameters:@{ @"MetricScale" : @"km" }];
                break;
            }
            case 1:
            {
                [TMAnalytics logEvent:@"Measurement.Options" withParameters:@{ @"MetricScale" : @"m" }];
                break;
            }
            case 2:
            {
                [TMAnalytics logEvent:@"Measurement.Options" withParameters:@{ @"MetricScale" : @"cm" }];
                break;
            }
            default:
                break;
        }
    }
    else
    {
        theMeasurement.unitsScaleImperial = self.btnScale.selectedSegmentIndex;
        
        switch (self.btnScale.selectedSegmentIndex)
        {
            case 0:
            {
                [TMAnalytics logEvent:@"Measurement.Options" withParameters:@{ @"ImperialScale" : @"mi" }];
                break;
            }
            case 1:
            {
                [TMAnalytics logEvent:@"Measurement.Options" withParameters:@{ @"ImperialScale" : @"yd" }];
                break;
            }
            case 2:
            {
                [TMAnalytics logEvent:@"Measurement.Options" withParameters:@{ @"ImperialScale" : @"in" }];
                break;
            }
            default:
                break;
        }
    }
    
    [delegate didChangeOptions];
}

- (IBAction)handleFractionalButton:(id)sender
{
    theMeasurement.fractional = (int32_t)self.btnFractional.selectedSegmentIndex;
    
    if (theMeasurement.fractional == 0)
        [TMAnalytics logEvent:@"Measurement.Options" withParameters:@{ @"Fractional" : @"Decimal" }];
    else
        [TMAnalytics logEvent:@"Measurement.Options" withParameters:@{ @"Fractional" : @"Fractional" }];
    
    [delegate didChangeOptions];
}

- (void)saveMeasurement
{
    NSError *error;
    [theMeasurement.managedObjectContext save:&error];
    if (error) [TMAnalytics logError:@"Options.Save" message:error.debugDescription error:error];
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch
{
    if(touch.view == self.containerView)
    {
        return NO;
    }
    else
    {
        return YES;
    }
}

- (void) handleTapOutside:(UIGestureRecognizer *)gestureRecognizer
{
    [self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
}

@end
