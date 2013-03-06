//
//  TMResultsVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMResultsVC.h"

@interface TMResultsVC ()

@end

@implementation TMResultsVC

@synthesize theMeasurement;

- (void)viewWillAppear:(BOOL)animated
{
    [self.navigationController setToolbarHidden:NO animated:animated];
    
    if ([self.prevView class] != [TMNewMeasurementVC class]) self.navigationItem.rightBarButtonItems = nil;
    
    [super viewWillAppear:animated];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
}

- (void)viewDidAppear:(BOOL)animated
{
    [self.tableView reloadData];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [self saveMeasurement];
}

- (void)viewDidUnload
{
    [theConnection cancel];
    [self setBtnDone:nil];
    [self setBtnAction:nil];
    [super viewDidUnload];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return NO;
}

- (IBAction)handleDeleteButton:(id)sender
{
    theMeasurement.deleted = YES;
    theMeasurement.syncPending = YES;
    [DATA_MANAGER saveContext];
    
    [theMeasurement
     putToServer:^(int transId) {
         NSLog(@"putMeasurement success callback");
         [theMeasurement deleteFromDb];
         [DATA_MANAGER saveContext];
     }
     onFailure:^(int statusCode) {
         NSLog(@"putMeasurement failure callback");
     }
     ];
    
    [self.navigationController popToRootViewControllerAnimated:YES];
}

- (IBAction)handleDoneButton:(id)sender
{
    [self.navigationController popToRootViewControllerAnimated:YES];
}

- (IBAction)handleKeyboardDone:(id)sender
{
    //no need to do anything here
}

- (IBAction)handleActionButton:(id)sender
{
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
    
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Coming Soon"
                                                    message:@"We're working on it!"
                                                   delegate:self
                                          cancelButtonTitle:@"OK"
                                          otherButtonTitles:nil];
    [alert show];
}

- (void) prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if([[segue identifier] isEqualToString:@"toOptions"])
    {
        TMOptionsVC *optionsVC = [segue destinationViewController];
        optionsVC.theMeasurement = theMeasurement;
        
        [[segue destinationViewController] setDelegate:self];
    }
    else if([[segue identifier] isEqualToString:@"toMap"])
    {
        TMMapVC *mapVC = [segue destinationViewController];
        mapVC.theMeasurement = theMeasurement;
    }
}

- (void)didDismissOptions
{
    [self.tableView reloadData];
}

- (NSString*)getLocationDisplayText:(TMLocation*)location
{
    if(!location) return @"";
    
    if(location.locationName.length > 0)
    {
        return location.locationName;
    }
    else if (location.address.length > 0)
    {
        return  location.address;
    }
    else
    {
        return [NSString localizedStringWithFormat:@"%f, %f", location.latititude, location.longitude];
    }
}

- (void)saveMeasurement
{
    UITextField *nameBox = (UITextField*)[[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]] viewWithTag:1];
    
    if (![theMeasurement.name isEqualToString:nameBox.text])
    {
        theMeasurement.name = nameBox.text;
        theMeasurement.syncPending = YES;
        [DATA_MANAGER saveContext];
        
        [theMeasurement
         putToServer:^(int transId) {
             NSLog(@"putMeasurement success callback");
             theMeasurement.syncPending = NO;
             [DATA_MANAGER saveContext];
         }
         onFailure:^(int statusCode) {
             NSLog(@"putMeasurement failure callback");
         }
         ];
    }
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    // Return the number of sections.
    return 2;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    // Return the number of rows in the section.
    if (section == 0) return 3;
    else if (section == 1) return 4;
    else return 0;
}

- (NSString *)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    // The header for the section is the region name -- get this from the region at the section index.
    if (section == 0) return @"Details";
    else if (section == 1) return @"Measurements";
    else return @"";
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell *cell;
    UILabel *label;

    if (indexPath.section == 0) //Details section
    {
        switch (indexPath.row)
        {
            case 0:
            {
                cell = [tableView dequeueReusableCellWithIdentifier:@"editTextCell"];

                label = (UILabel*)[cell viewWithTag:2];
                UITextField *name = (UITextField*)[cell viewWithTag:1];
                
                label.text = @"Name";
                name.text = theMeasurement.name;
                
                name.delegate = self; //to handle done button
                
                break;
            }
            case 1:
            {
                cell = [tableView dequeueReusableCellWithIdentifier:@"plainCell"];
                
                label = (UILabel*)[cell viewWithTag:2];
                UILabel *date = (UILabel*)[cell viewWithTag:1];
                
                label.text = @"Date";
                date.text = [NSDateFormatter localizedStringFromDate:[NSDate dateWithTimeIntervalSince1970:theMeasurement.timestamp]
                                                           dateStyle:NSDateFormatterShortStyle timeStyle:NSDateFormatterShortStyle];
                
                break;
            }
            case 2:
            {
                cell = [tableView dequeueReusableCellWithIdentifier:@"locationCell"];
                
                label = (UILabel*)[cell viewWithTag:2];
                UILabel *locationName = (UILabel*)[cell viewWithTag:1];
                
                TMLocation *location = (TMLocation*)theMeasurement.location;
                
                label.text = @"Location";
                locationName.text = [self getLocationDisplayText:location];
                
                break;
            }
            default:
                break;
        }
    }
    else if (indexPath.section == 1)  //Measurements section. Ordered with primary measurement type first.
    {
        cell = [self getMeasurementCell:indexPath.row forMeasurementType:(MeasurementType)theMeasurement.type];
    }
    
    return cell;
}

//could be more elegant, but this was easier and more flexible
- (UITableViewCell*)getMeasurementCell:(int)rowNum forMeasurementType:(MeasurementType)type
{  
    switch (type) {
        case TypePointToPoint:
        {
            switch (rowNum)
            {
                case 0: return [self getMeasurementCell:@"Point to Point" withValue:theMeasurement.pointToPoint isPrimary:YES];
                case 1: return [self getMeasurementCell:@"Total Path" withValue:theMeasurement.totalPath]; 
                case 2: return [self getMeasurementCell:@"Horizontal" withValue:theMeasurement.horzDist]; 
                case 3: return [self getMeasurementCell:@"Vertical" withValue:theMeasurement.vertDist]; 
                default: break;
            }
            break;
        }
        case TypeTotalPath:
        {
            switch (rowNum)
            {
                case 0: return [self getMeasurementCell:@"Total Path" withValue:theMeasurement.totalPath isPrimary:YES];
                case 1: return [self getMeasurementCell:@"Point to Point" withValue:theMeasurement.pointToPoint];
                case 2: return [self getMeasurementCell:@"Horizontal" withValue:theMeasurement.horzDist];
                case 3: return [self getMeasurementCell:@"Vertical" withValue:theMeasurement.vertDist];
                default: break;
            }
            break;
        }
        case TypeHorizontal:
        {
            switch (rowNum)
            {
                case 0: return [self getMeasurementCell:@"Horizontal" withValue:theMeasurement.horzDist isPrimary:YES];
                case 1: return [self getMeasurementCell:@"Vertical" withValue:theMeasurement.vertDist];
                case 2: return [self getMeasurementCell:@"Point to Point" withValue:theMeasurement.pointToPoint];
                case 3: return [self getMeasurementCell:@"Total Path" withValue:theMeasurement.totalPath];
                default: break;
            }
            break;
        }
        case TypeVertical:
        {
            switch (rowNum)
            {
                case 0: return [self getMeasurementCell:@"Vertical" withValue:theMeasurement.vertDist isPrimary:YES];
                case 1: return [self getMeasurementCell:@"Horizontal" withValue:theMeasurement.horzDist];
                case 2: return [self getMeasurementCell:@"Point to Point" withValue:theMeasurement.pointToPoint];
                case 3: return [self getMeasurementCell:@"Total Path" withValue:theMeasurement.totalPath];
                default: break;
            }
            break;
        }
        default:
            break;
    }
    
    return [self.tableView dequeueReusableCellWithIdentifier:@"measurementCell"]; //return blank cell by default
}

- (UITableViewCell*)getMeasurementCell:(NSString*)labelText withValue:(float) measurementValue
{
    return [self getMeasurementCell:labelText withValue:measurementValue isPrimary:NO];
}

- (UITableViewCell*)getMeasurementCell:(NSString*)labelText withValue:(float) measurementValue isPrimary:(bool)isPrimary
{
    UITableViewCell *cell;
    
    if (isPrimary)
    {
        cell = [self.tableView dequeueReusableCellWithIdentifier:@"measurementCellPrimary"];
    }
    else
    {
        cell = [self.tableView dequeueReusableCellWithIdentifier:@"measurementCell"];
    }
    
    UILabel *label = (UILabel*)[cell viewWithTag:2];
    UILabel *measurementValueText = (UILabel*)[cell viewWithTag:1];
    
    label.text = labelText;
    measurementValueText.text = [theMeasurement getFormattedDistance:measurementValue];
    
    return cell;
}

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView accessoryButtonTappedForRowWithIndexPath:(NSIndexPath *)indexPath
{
     NSLog(@"selected: %@", indexPath);
    [self performSegueWithIdentifier:@"toMap" sender:self];
}

@end
