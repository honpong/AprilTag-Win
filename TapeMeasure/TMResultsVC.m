//
//  TMResultsVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMResultsVC.h"
#import <UIKit/UIKit.h>
#import <QuartzCore/QuartzCore.h>
#import "TMMeasurement.h"

@interface TMResultsVC ()

@end

@implementation TMResultsVC

@synthesize theMeasurement = _theMeasurement, theName = _theName, theDate = _theDate, pointToPoint = _pointToPoint, totalPath = _totalPath, horzDist = _horzDist, vertDist = _vertDist;

- (void)viewDidLoad
{
    [super viewDidLoad];
	
    //give button rounded corners
	CALayer *layer = [self.upgradeBtn layer];
    [layer setMasksToBounds:YES];
    [layer setCornerRadius:10.0];
    [layer setBorderWidth:1.0];
    [layer setBorderColor:[[UIColor grayColor] CGColor]];
    
    NSLog(@"Measurement name: %@", _theMeasurement.name);
    
    [self configureView];
}

- (void)configureView
{
    static NSDateFormatter *formatter = nil;
    if (formatter == nil) {
        formatter = [[NSDateFormatter alloc] init];
        [formatter setDateStyle:NSDateFormatterMediumStyle];
    }
    
    self.theName.text = self.theMeasurement.name;
    self.theDate.text = [formatter stringFromDate:self.theMeasurement.timestamp];
    self.pointToPoint.text = [NSString stringWithFormat:@"%@\"", self.theMeasurement.pointToPoint];
    self.totalPath.text = [NSString stringWithFormat:@"%@\"", self.theMeasurement.totalPath];
    self.horzDist.text = [NSString stringWithFormat:@"%@\"", self.theMeasurement.horzDist];
    self.vertDist.text = [NSString stringWithFormat:@"%@\"", self.theMeasurement.vertDist];
}

- (void)viewDidUnload {
    [self setUpgradeBtn:nil];
    [self setBtnDone:nil];
    [self setPointToPoint:nil];
    [self setTotalPath:nil];
    [self setHorzDist:nil];
    [self setVertDist:nil];
    [self setTheName:nil];
    [self setTheDate:nil];
    [super viewDidUnload];
}

- (IBAction)handleDoneButton:(id)sender {
    [self.navigationController popToRootViewControllerAnimated:YES];
}

- (IBAction)handleDeleteButton:(id)sender {
}

- (IBAction)handleUpgradeButton:(id)sender {
}



@end
