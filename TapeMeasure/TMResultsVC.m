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

@synthesize theMeasurement = _theMeasurement, nameBox = _nameBox, theDate = _theDate, pointToPoint = _pointToPoint, totalPath = _totalPath, horzDist = _horzDist, vertDist = _vertDist;

- (void)viewDidLoad
{
    [super viewDidLoad];
	
    NSMutableArray *navigationArray = [[NSMutableArray alloc] initWithArray: self.navigationController.viewControllers];
    
    if(navigationArray.count == 3) //if this is third screen in the stack
    {
        [navigationArray removeObjectAtIndex: 1];  // remove New Measurement VC from nav array, so back button goes to history instead
        self.navigationController.viewControllers = navigationArray;
    }
    
    [self configureView];
}

- (void)configureView
{
    static NSDateFormatter *formatter = nil;
    if (formatter == nil) {
        formatter = [[NSDateFormatter alloc] init];
        [formatter setDateStyle:NSDateFormatterMediumStyle];
    }
    
    self.nameBox.text = self.theMeasurement.name;
    self.theDate.text = [[NSDateFormatter class] localizedStringFromDate:self.theMeasurement.timestamp dateStyle:NSDateFormatterShortStyle timeStyle:NSDateFormatterShortStyle];
    self.pointToPoint.text = [NSString stringWithFormat:@"%@\"", self.theMeasurement.pointToPoint];
    self.totalPath.text = [NSString stringWithFormat:@"%@\"", self.theMeasurement.totalPath];
    self.horzDist.text = [NSString stringWithFormat:@"%@\"", self.theMeasurement.horzDist];
    self.vertDist.text = [NSString stringWithFormat:@"%@\"", self.theMeasurement.vertDist];
}

- (void)viewDidUnload {
    [self setPointToPoint:nil];
    [self setTotalPath:nil];
    [self setHorzDist:nil];
    [self setVertDist:nil];
    [self setTheDate:nil];
    [self setNameBox:nil];
    [self setBtnDone:nil];
    [super viewDidUnload];
}

- (IBAction)handleDeleteButton:(id)sender {
    [self.theMeasurement.managedObjectContext deleteObject:self.theMeasurement];
    [self.navigationController popToRootViewControllerAnimated:YES];
}

- (IBAction)handleUpgradeButton:(id)sender {
}

- (IBAction)handleDoneButton:(id)sender {
    
    NSError *error;
    
    self.theMeasurement.name = self.nameBox.text;
    [self.theMeasurement.managedObjectContext save:&error]; //TODO: Handle save error
    
    [self.navigationController popToRootViewControllerAnimated:YES];
}



@end
