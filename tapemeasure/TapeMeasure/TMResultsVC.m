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
    if ([self.prevView class] == [TMNewMeasurementVC class]) self.navigationItem.backBarButtonItem = nil;
    
    [super viewWillAppear:animated];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [RCDistanceLabel class]; // needed so that storyboard can see this class, since it's in a library
    [OSKActivitiesManager sharedInstance].customizationsDelegate = self;
}

- (void)viewDidAppear:(BOOL)animated
{
    [TMAnalytics logEvent:@"View.Results"];
    [self.tableView reloadData];
    [super viewDidAppear:animated];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [self saveMeasurement];
    [super viewWillDisappear:animated];
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
    
    [TMAnalytics logEvent:@"Measurement.Delete.Results"];
    
    [theMeasurement
     putToServer:^(NSInteger transId) {
         DLog(@"putMeasurement success callback");
         [theMeasurement deleteFromDb];
         [DATA_MANAGER saveContext];
     }
     onFailure:^(NSInteger statusCode) {
         DLog(@"putMeasurement failure callback");
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
//    [self showActionSheet];
    [self showShareSheet];
}

- (IBAction)handlePageCurl:(id)sender
{
    [TMAnalytics logEvent:@"Measurement.ViewOptions.Results"];
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
    DLog(@"Button %ld", (long)buttonIndex);
    
    if (buttonIndex != 4)
    {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Coming Soon"
                                                        message:@"We're working on it!"
                                                       delegate:self
                                              cancelButtonTitle:@"OK"
                                              otherButtonTitles:nil];
        [alert show];
    }
}

- (void) prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if([[segue identifier] isEqualToString:@"toOptions"])
    {
        [self saveMeasurement]; // we do this here because viewWillDisappear doesn't get called when a modal view appears on top of it
        TMOptionsVC *optionsVC = [segue destinationViewController];
        optionsVC.theMeasurement = theMeasurement;
        [optionsVC setDelegate:self];
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
        return [NSString localizedStringWithFormat:@"%3.3f, %3.3f", location.latititude, location.longitude];
    }
}

- (void)saveMeasurement
{
    UITextField *nameBox = (UITextField*)[[self.tableView cellForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]] viewWithTag:1];
    
    if (![theMeasurement.name isEqualToString:nameBox.text] && !theMeasurement.deleted)
    {
        theMeasurement.name = nameBox.text;
        theMeasurement.syncPending = YES;
        [DATA_MANAGER saveContext];
        
        [TMAnalytics logEvent:@"Measurement.Edit"];
        
        if (theMeasurement.dbid > 0) {
            [theMeasurement
             putToServer:^(NSInteger transId) {
                 DLog(@"PUT measurement success callback");
                 theMeasurement = (TMMeasurement*)[DATA_MANAGER getObjectOfType:[TMMeasurement getEntity] byDbid:theMeasurement.dbid];
                 if (theMeasurement)
                 {
                     theMeasurement.syncPending = NO;
                     [DATA_MANAGER saveContext];
                 }
                 else
                 {
                     DLog(@"Failed to save measurement. Measurement not found.");
                 }
             }
             onFailure:^(NSInteger statusCode) {
                 DLog(@"PUT measurement failure callback");
             }
             ];
        }
        else
        {
            [theMeasurement
             postToServer:^(NSInteger transId) {
                 DLog(@"POST measurement success callback");
                 theMeasurement = (TMMeasurement*)[DATA_MANAGER getObjectOfType:[TMMeasurement getEntity] byDbid:theMeasurement.dbid];
                 if (theMeasurement)
                 {
                     theMeasurement.syncPending = NO;
                     [DATA_MANAGER saveContext];
                 }
                 else
                 {
                     DLog(@"Failed to save measurement. Measurement not found.");
                 }
             }
             onFailure:^(NSInteger statusCode) {
                 DLog(@"POST measurement failure callback");
             }
             ];
        }
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
                date.text = [theMeasurement getLocalizedDateString];
                
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
- (UITableViewCell*)getMeasurementCell:(NSInteger)rowNum forMeasurementType:(MeasurementType)type
{  
    switch (type) {
        case TypePointToPoint:
        {
            switch (rowNum)
            {
                case 0: return [self getMeasurementCell:@"Point to Point" withValue:theMeasurement.pointToPoint isPrimary:YES];
                case 1: return [self getMeasurementCell:@"Total Path" withValue:theMeasurement.totalPath]; 
                case 2: return [self getMeasurementCell:@"Horizontal" withValue:theMeasurement.horzDist]; 
                case 3: return [self getMeasurementCell:@"Vertical" withValue:theMeasurement.zDisp]; 
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
                case 3: return [self getMeasurementCell:@"Vertical" withValue:theMeasurement.zDisp];
                default: break;
            }
            break;
        }
        case TypeHorizontal:
        {
            switch (rowNum)
            {
                case 0: return [self getMeasurementCell:@"Horizontal" withValue:theMeasurement.horzDist isPrimary:YES];
                case 1: return [self getMeasurementCell:@"Vertical" withValue:theMeasurement.zDisp];
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
                case 0: return [self getMeasurementCell:@"Vertical" withValue:theMeasurement.zDisp isPrimary:YES];
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
    label.text = labelText;
    
    RCDistanceLabel *distLabel = (RCDistanceLabel*)[cell viewWithTag:1];
    [distLabel setDistance:[theMeasurement getDistanceObject:measurementValue]];
    
    return cell;
}

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView accessoryButtonTappedForRowWithIndexPath:(NSIndexPath *)indexPath
{
     DLog(@"selected: %@", indexPath);
    [self performSegueWithIdentifier:@"toMap" sender:self];
}

#pragma mark - Sharing

- (NSString*) composeSharingString
{
    NSString *name, *madeWith;
    
    if (theMeasurement.name && theMeasurement.name.length > 0)
        name = theMeasurement.name;
    else
        name = @"N/A";
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
        madeWith = @"Measured on my iPad with Endless Tape Measure\nhttp://realitycap.com";
    else
        madeWith = @"Measured on my iPhone with Endless Tape Measure\nhttp://realitycap.com";
    
    NSString* result = [NSString
                        stringWithFormat:@"%@: %@\n%@\n\n%@",
                        name,
                        [theMeasurement getPrimaryDistanceObject],
                        [theMeasurement getLocalizedDateString],
                        madeWith];
    
    return result;
}

- (void) showShareSheet
{
    OSKShareableContent *content = [OSKShareableContent contentFromText:[self composeSharingString]];
    content.title = @"Share Measurement";
    
    if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone) {
        [self showShareSheet_Phone:content];
    } else {
        [self showShareSheet_Pad_FromBarButtonItem:self.btnAction content:content];
    }
}

- (void) showShareSheet_Pad_FromBarButtonItem:(UIBarButtonItem *)barButtonItem content:(OSKShareableContent *)content
{
    // 2) Setup optional completion and dismissal handlers
    OSKActivityCompletionHandler completionHandler = [self activityCompletionHandler];
    OSKPresentationEndingHandler dismissalHandler = [self dismissalHandler];
    
    // 3) Create the options dictionary. See OSKActivity.h for more options.
    NSDictionary *options = @{    OSKPresentationOption_ActivityCompletionHandler : completionHandler,
                                  OSKPresentationOption_PresentationEndingHandler : dismissalHandler};
    
    // 4) Present the activity sheet via the presentation manager.
    [[OSKPresentationManager sharedInstance] presentActivitySheetForContent:content
                                                   presentingViewController:self
                                                   popoverFromBarButtonItem:barButtonItem
                                                   permittedArrowDirections:UIPopoverArrowDirectionAny
                                                                   animated:YES
                                                                    options:options];
}

- (void) showShareSheet_Phone:(OSKShareableContent *)content
{
    // 2) Setup optional completion and dismissal handlers
    OSKActivityCompletionHandler completionHandler = [self activityCompletionHandler];
    OSKPresentationEndingHandler dismissalHandler = [self dismissalHandler];
    
    // 3) Create the options dictionary. See OSKActivity.h for more options.
    NSDictionary *options = @{    OSKPresentationOption_ActivityCompletionHandler : completionHandler,
                                  OSKPresentationOption_PresentationEndingHandler : dismissalHandler};
    
    // 4) Present the activity sheet via the presentation manager.
    [[OSKPresentationManager sharedInstance] presentActivitySheetForContent:content
                                                   presentingViewController:self
                                                                    options:options];
}

- (OSKActivityCompletionHandler)activityCompletionHandler
{
    OSKActivityCompletionHandler activityCompletionHandler = ^(OSKActivity *activity, BOOL successful, NSError *error)
    {
        // placeholder
    };
    return activityCompletionHandler;
}

- (OSKPresentationEndingHandler) dismissalHandler
{
    __weak TMResultsVC *weakSelf = self;
    OSKPresentationEndingHandler dismissalHandler = ^(OSKPresentationEnding ending, OSKActivity *activityOrNil){
        OSKLog(@"Sheet dismissed.");
        if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad) {
            // placeholder
        }
    };
    return dismissalHandler;
}

- (OSKApplicationCredential *)applicationCredentialForActivityType:(NSString *)activityType
{
    OSKApplicationCredential *appCredential = nil;

    if ([activityType isEqualToString:OSKActivityType_iOS_Facebook]) {
        appCredential = [[OSKApplicationCredential alloc]
                         initWithOvershareApplicationKey:RCApplicationCredential_Facebook_Key
                         applicationSecret:nil
                         appName:@"Overshare"];
    }
    else if ([activityType isEqualToString:OSKActivityType_API_GooglePlus]) {
        appCredential = [[OSKApplicationCredential alloc]
                         initWithOvershareApplicationKey:RCApplicationCredential_GooglePlus_Key
                         applicationSecret:nil
                         appName:@"Overshare"];
    }
    
    return appCredential;
}

@end
