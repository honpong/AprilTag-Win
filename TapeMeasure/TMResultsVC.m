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
    
    self.nameBox.delegate = self; //handle done button on keyboard
    
    [self configureView];
}

- (void)configureView
{
    self.nameBox.text = self.theMeasurement.name;
    self.theDate.text = [[NSDateFormatter class] localizedStringFromDate:self.theMeasurement.timestamp dateStyle:NSDateFormatterShortStyle timeStyle:NSDateFormatterShortStyle];
    self.pointToPoint.text = [NSString localizedStringWithFormat:@"%0.1f\"", self.theMeasurement.pointToPoint.floatValue];
    self.totalPath.text = [NSString localizedStringWithFormat:@"%0.1f\"", self.theMeasurement.totalPath.floatValue];
    self.horzDist.text = [NSString localizedStringWithFormat:@"%0.1f\"", self.theMeasurement.horzDist.floatValue];
    self.vertDist.text = [NSString localizedStringWithFormat:@"%0.1f\"", self.theMeasurement.vertDist.floatValue];
}

- (void)viewDidUnload {
    [self setPointToPoint:nil];
    [self setTotalPath:nil];
    [self setHorzDist:nil];
    [self setVertDist:nil];
    [self setTheDate:nil];
    [self setNameBox:nil];
    [self setBtnDone:nil];
    [self setBtnAction:nil];
    [super viewDidUnload];
}


- (BOOL)textFieldShouldReturn:(UITextField *)textField {
    [textField resignFirstResponder];
    return NO;
}

- (IBAction)handleDeleteButton:(id)sender {
    NSError *error;
    
    [self.theMeasurement.managedObjectContext deleteObject:self.theMeasurement];
    [self.theMeasurement.managedObjectContext save:&error]; //TODO: Handle save error
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

- (IBAction)handleKeyboardDone:(id)sender {
    //no need to do anything here
}

- (IBAction)handleActionButton:(id)sender {
    [self showActionSheet];
}

- (void)showActionSheet
{
    sheet = [[UIActionSheet alloc] initWithTitle:@"Share this measurement"
                                        delegate:self
                               cancelButtonTitle:@"Cancel"
                                destructiveButtonTitle:nil
                               otherButtonTitles:@"Facebook", @"Twitter", @"Email", @"Text Message", nil];
    // Show the sheet
    [sheet showFromToolbar:self.navigationController.toolbar];
}

- (void)actionSheet:(UIActionSheet *)actionSheet didDismissWithButtonIndex:(NSInteger)buttonIndex
{
    NSLog(@"Button %d", buttonIndex);
}



@end
