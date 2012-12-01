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
#import "TMDistanceFormatter.h"

@interface TMResultsVC ()

@end

@implementation TMResultsVC

@synthesize theMeasurement, nameBox, theDate, pointToPoint, totalPath, horzDist, vertDist;

- (void)viewWillAppear:(BOOL)animated
{
    [self.navigationController setToolbarHidden:NO animated:animated];
    [super viewWillAppear:animated];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	
//    NSMutableArray *navigationArray = [[NSMutableArray alloc] initWithArray: self.navigationController.viewControllers];
//    
//    if(navigationArray.count == 3) //if this is third screen in the stack
//    {
//        [navigationArray removeObjectAtIndex: 1];  // remove New Measurement VC from nav array, so back button goes to history instead
//        self.navigationController.viewControllers = navigationArray;
//    }

    nameBox.delegate = self; //handle done button on keyboard
    
    [self configureView];
}

- (void)configureView
{
    NSLog(@"configureView");
    
//    [theMeasurement.managedObjectContext refreshObject:theMeasurement mergeChanges:YES];
    
    nameBox.text = theMeasurement.name;
    theDate.text = [[NSDateFormatter class] localizedStringFromDate:theMeasurement.timestamp dateStyle:NSDateFormatterShortStyle timeStyle:NSDateFormatterShortStyle];
    pointToPoint.text = [TMDistanceFormatter formattedDistance:theMeasurement.pointToPoint withMeasurement:theMeasurement];
    totalPath.text = [TMDistanceFormatter formattedDistance:theMeasurement.totalPath withMeasurement:theMeasurement];
    horzDist.text = [TMDistanceFormatter formattedDistance:theMeasurement.horzDist withMeasurement:theMeasurement];
    vertDist.text = [TMDistanceFormatter formattedDistance:theMeasurement.vertDist withMeasurement:theMeasurement];
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
    
    [theMeasurement.managedObjectContext deleteObject:theMeasurement];
    [theMeasurement.managedObjectContext save:&error]; //TODO: Handle save error
    [self.navigationController popToRootViewControllerAnimated:YES];
}

- (IBAction)handleUpgradeButton:(id)sender {
}

- (IBAction)handleDoneButton:(id)sender {
    
    theMeasurement.name = nameBox.text;
    
    [self saveMeasurement];
    
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

- (void) prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if([[segue identifier] isEqualToString:@"toOptions"])
    {
        TMResultsVC *resultsVC = [segue destinationViewController];
        resultsVC.theMeasurement = theMeasurement;
        
        [[segue destinationViewController] setDelegate:self];
    }
}

- (void)didDismissOptions
{
    [self configureView];
}

- (void)saveMeasurement
{
    NSError *error;
    [theMeasurement.managedObjectContext save:&error]; //TODO: Handle save error
}

@end
