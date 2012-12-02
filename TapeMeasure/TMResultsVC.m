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

- (void)viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    //remove back button. it's unneeded because we have a Done button which takes us back to the history screen.
    //it also solves the problem of the back button saying "Cancel".
    self.navigationItem.hidesBackButton = YES;
}

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
    
    [self postMeasurement];
    
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

-(void)postMeasurement
{
    NSString *bodyData = [NSString stringWithFormat:@"measurement[user_id]=1&measurement[name]=%@&measurement[value]=%@&measurement[location_id]=3", [self urlEncodeString:self.theMeasurement.name], self.theMeasurement.pointToPoint];
    
    NSMutableURLRequest *postRequest = [NSMutableURLRequest requestWithURL:[NSURL URLWithString:@"http://192.168.1.1:3000/measurements.json"]];
    
    [postRequest setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
    [postRequest setHTTPMethod:@"POST"];
    [postRequest setHTTPBody:[NSData dataWithBytes:[bodyData UTF8String] length:[bodyData length]]];
    
    NSURLConnection *theConnection=[[NSURLConnection alloc] initWithRequest:postRequest delegate:self];
    
    if (theConnection)
    {
        NSLog(@"Connection ok");
        //        receivedData = [[NSMutableData data] retain];
    }
    else
    {
        NSLog(@"Connection failed");
    }
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response
{
    
    // This method is called when the server has determined that it
    // has enough information to create the NSURLResponse.
    
    // It can be called multiple times, for example in the case of a
    // redirect, so each time we reset the data.
    
    // receivedData is an instance variable declared elsewhere.
    
    //    [receivedData setLength:0];
    
    NSLog(@"didReceiveResponse");
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)data

{
    // Append the new data to receivedData.
    // receivedData is an instance variable declared elsewhere.
    
    //    [receivedData appendData:data];
    
    NSLog(@"didReceiveData");
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)error
{
    NSLog(@"Connection failed! Error - %@ %@",
          [error localizedDescription],
          [[error userInfo] objectForKey:NSURLErrorFailingURLStringErrorKey]);
}

-(NSString*)urlEncodeString:(NSString*)input
{
    NSString *urlEncoded = (__bridge_transfer NSString *)CFURLCreateStringByAddingPercentEscapes(
                                                                                                 NULL,
                                                                                                 (__bridge CFStringRef)input,
                                                                                                 NULL,
                                                                                                 (CFStringRef)@"!*'\"();:@&=+$,/?%#[]% ",
                                                                                                 kCFStringEncodingUTF8);
    return urlEncoded;
}


@end
