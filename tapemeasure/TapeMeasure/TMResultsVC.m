//
//  TMResultsVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/2/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMResultsVC.h"
#import "TMHistoryVC.h"
#import <RCCore/RCCore.h>

@interface TMResultsVC ()

@end

@implementation TMResultsVC
{
    RCRateMeView* rateMeView;
    RCLocationPopUp* locationPopup;
    TMShareSheet* shareSheet;
    UIView* dimmingView;
    RCModalCoverVerticalTransition* transition;
}
@synthesize theMeasurement;

- (void)viewWillAppear:(BOOL)animated
{
    [self.navigationController setToolbarHidden:NO animated:animated];    
    if ([self.presentingViewController isKindOfClass:[TMNewMeasurementVC class]]) self.navigationItem.backBarButtonItem = nil;
    
    [super viewWillAppear:animated];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [self.navigationController setNavigationBarHidden:NO animated:NO]; // necessary because location intro may have hidden it
    
    [RCDistanceLabel class]; // needed so that storyboard can see this class, since it's in a library
    CGFloat fontSize = UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad ? 70 : 50;
    self.distLabel.font = [UIFont systemFontOfSize:fontSize];
    self.distLabel.textColor = [UIColor colorWithRed:219./255. green:166./255. blue:46./255. alpha:1.];
    self.distLabel.shadowColor = [UIColor darkGrayColor];
    self.distLabel.textAlignment = NSTextAlignmentCenter;
    [self.distLabel setDistance:theMeasurement.getPrimaryDistanceObject];
    
    [self createRateMeBanner];
    [self createLocationPopup];
    
    dimmingView = [UIView new];
    dimmingView.backgroundColor = [UIColor colorWithRed:0 green:0 blue:0 alpha:.5];
}

- (void)viewDidAppear:(BOOL)animated
{
    [TMAnalytics logEvent:@"View.Results"];
    [self.tableView reloadData];
    [super viewDidAppear:animated];
    
    if (![self showLocationNagIfNecessary])
        [self showRateNagIfNecessary];
}

- (void)viewWillDisappear:(BOOL)animated
{
    [locationPopup removeFromSuperview];
    [rateMeView removeFromSuperview];
    [rateMeView hideInstantly];
    [self saveMeasurement];
    [super viewWillDisappear:animated];
}

- (void)viewDidUnload
{
    [theConnection cancel];
    [self setBtnNew:nil];
    [self setBtnAction:nil];
    [super viewDidUnload];
}

- (void) viewDidLayoutSubviews
{
    [self.navigationController.view bringSubviewToFront:rateMeView];
}

#pragma mark - UITextFieldDelegate

- (void)textFieldDidEndEditing:(UITextField *)textField
{
    if (textField.text.length > 0) [TMAnalytics logEvent:@"Measurement.NameEdited"];
}

- (BOOL)textFieldShouldReturn:(UITextField *)textField
{
    [textField resignFirstResponder];
    return NO;
}

#pragma mark -

- (IBAction)handleDeleteButton:(id)sender
{
    theMeasurement.deleted = YES;
    theMeasurement.syncPending = YES;
    [DATA_MANAGER saveContext];
    
    [TMAnalytics logEvent:@"Measurement.Delete.Results"];
    
//    [theMeasurement
//     putToServer:^(NSInteger transId) {
//         DLog(@"putMeasurement success callback");
//         [theMeasurement deleteFromDb];
//         [DATA_MANAGER saveContext];
//     }
//     onFailure:^(NSInteger statusCode) {
//         DLog(@"putMeasurement failure callback");
//     }
//     ];
    
    [self.navigationController popViewControllerAnimated:YES];
}

- (IBAction)handleNewButton:(id)sender
{
    [self saveMeasurement];
    [self.navigationController popToRootViewControllerAnimated:NO];
}

- (IBAction)handleListButton:(id)sender
{
    [self saveMeasurement];
    
    if ([self.navigationController.secondToLastViewController isKindOfClass:[TMHistoryVC class]])
    {
        [self.navigationController popViewControllerAnimated:YES];
    }
    else
    {
        UIViewController* vc = [self.navigationController.storyboard instantiateViewControllerWithIdentifier:@"History"];
        [self.navigationController pushViewController:vc animated:YES];
    }
}

- (IBAction)handleKeyboardDone:(id)sender
{
    //no need to do anything here
}

- (IBAction)handleActionButton:(id)sender
{
    [TMAnalytics logEvent:@"View.ShareMeasurement"];
    
    OSKShareableContent *content = [OSKShareableContent contentFromText:[self composeSharingString]];
    content.title = @"Share Measurement";
    shareSheet = [TMShareSheet shareSheetWithDelegate:self];
    [OSKActivitiesManager sharedInstance].customizationsDelegate = shareSheet;
    [shareSheet showShareSheet_Pad_FromBarButtonItem:self.btnAction content:content];
}

- (IBAction)handleUnitsButton:(id)sender
{
    TMOptionsVC *optionsVC = [self.storyboard instantiateViewControllerWithIdentifier:@"Options"];
    optionsVC.theMeasurement = theMeasurement;
    [optionsVC setDelegate:self];
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPhone)
    {
        if (transition == nil) transition = [RCModalCoverVerticalTransition new];
        optionsVC.transitioningDelegate = transition;
        optionsVC.modalPresentationStyle = UIModalPresentationCustom;
    }

    [self presentViewController:optionsVC animated:YES completion:nil];
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

#pragma mark - TMOptionsDelegate

- (void) didChangeOptions
{
    [self.distLabel setDistance:theMeasurement.getPrimaryDistanceObject]; // update label with new units
}

- (void) didDismissOptions
{
    [self unDimScreen];
}

#pragma mark -

- (void) createRateMeBanner
{
    rateMeView = [RCRateMeView new];
    [self.navigationController.view addSubview:rateMeView];
    [rateMeView addCenterXInSuperviewConstraints];
    [rateMeView setBottomSpaceToSuperviewConstraint:55];
    [rateMeView hideInstantly];
    rateMeView.delegate = self;
}

- (void) createLocationPopup
{
    locationPopup = [RCLocationPopUp new];
    [locationPopup setPrimaryColor: [UIColor colorWithRed:0 green:128./255. blue:0 alpha:1.]];
    [self.navigationController.view addSubview:locationPopup];
    [locationPopup addCenterXInSuperviewConstraints];
    [locationPopup setBottomSpaceToSuperviewConstraint:55];
    [locationPopup hideInstantly];
    locationPopup.delegate = self;
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
                
//        if ([USER_MANAGER getLoginState] == LoginStateYes)
//        {
//            if (theMeasurement.dbid > 0) {
//                [theMeasurement
//                 putToServer:^(NSInteger transId) {
//                     DLog(@"PUT measurement success callback");
//                     theMeasurement = (TMMeasurement*)[DATA_MANAGER getObjectOfType:[TMMeasurement getEntity] byDbid:theMeasurement.dbid];
//                     if (theMeasurement)
//                     {
//                         theMeasurement.syncPending = NO;
//                         [DATA_MANAGER saveContext];
//                     }
//                     else
//                     {
//                         DLog(@"Failed to save measurement. Measurement not found.");
//                     }
//                 }
//                 onFailure:^(NSInteger statusCode) {
//                     DLog(@"PUT measurement failure callback");
//                 }
//                 ];
//            }
//            else
//            {
//                [theMeasurement
//                 postToServer:^(NSInteger transId) {
//                     DLog(@"POST measurement success callback");
//                     theMeasurement = (TMMeasurement*)[DATA_MANAGER getObjectOfType:[TMMeasurement getEntity] byDbid:theMeasurement.dbid];
//                     if (theMeasurement)
//                     {
//                         theMeasurement.syncPending = NO;
//                         [DATA_MANAGER saveContext];
//                     }
//                     else
//                     {
//                         DLog(@"Failed to save measurement. Measurement not found.");
//                     }
//                 }
//                 onFailure:^(NSInteger statusCode) {
//                     DLog(@"POST measurement failure callback");
//                 }
//                 ];
//            }
//        }
    }
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    // Return the number of sections.
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    // Return the number of rows in the section.
    if (section == 0) return 3;
//    else if (section == 1) return 4;
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
    NSString *name, *madeWith, *deviceType;
    
    if (theMeasurement.name && theMeasurement.name.length > 0)
        name = theMeasurement.name;
    else
        name = @"Untitled Measurement";
    
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
        deviceType = @"iPad";
    else
        deviceType = @"iPhone";
    
    madeWith = [NSString stringWithFormat: @"Measured on my %@ with Endless Tape Measure\n%@", deviceType, URL_SHARING];
    
    NSString* result = [NSString
                        stringWithFormat:@"%@: %@\n%@\n\n%@",
                        name,
                        [theMeasurement getPrimaryDistanceObject],
                        [theMeasurement getLocalizedDateString],
                        madeWith];
    
    return result;
}

#pragma mark - TMShareSheetDelegate

- (OSKActivityCompletionHandler) activityCompletionHandler
{
    OSKActivityCompletionHandler activityCompletionHandler = ^(OSKActivity *activity, BOOL successful, NSError *error){
        if (successful) {
            [TMAnalytics logEvent:@"Share.Measurement" withParameters:@{ @"Type": [activity.class activityName] }];
        } else {
            [TMAnalytics logError:@"Share.Measurement" message:[activity.class activityName] error:error];
        }
    };
    return activityCompletionHandler;
}

#pragma mark - RCRateMeViewDelegate

- (void) handleRateNowButton
{
    [TMAnalytics logEvent:@"Choice.RateNag" withParameters:@{ @"Button" : @"Rate" }];
    
    [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_SHOW_RATE_NAG];
    [rateMeView hideInstantly];
    [self gotoAppStore];
}

- (void) handleRateLaterButton
{
    [TMAnalytics logEvent:@"Choice.RateNag" withParameters:@{ @"Button" : @"Later" }];
    
    [rateMeView hideAnimated];
}

- (void) handleRateNeverButton
{
    [TMAnalytics logEvent:@"Choice.RateNag" withParameters:@{ @"Button" : @"Never" }];
    
    [rateMeView hideAnimated];
    [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_SHOW_RATE_NAG];
}

#pragma mark - RCLocationPopUpDelegate

- (void) handleAllowLocationButton
{
    [TMAnalytics logEvent:@"Choice.LocationNag" withParameters:@{ @"Button" : @"Allow" }];
    
    NSNumber* timestamp = [NSNumber numberWithDouble:[[NSDate date] timeIntervalSince1970]];
    [NSUserDefaults.standardUserDefaults setObject:timestamp forKey:PREF_LOCATION_NAG_TIMESTAMP];
    
    if (![CLLocationManager locationServicesEnabled])
    {
        [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_PROMPTED_LOCATION_SERVICES];
    }
    
    [locationPopup hideInstantly];
    [LOCATION_MANAGER requestLocationAccessWithCompletion:^(BOOL authorized) {
        [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_SHOW_LOCATION_NAG];
        [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_USE_LOCATION];
        if(authorized) [LOCATION_MANAGER startLocationUpdates];
    }];
}

- (void) handleLaterLocationButton
{
    [TMAnalytics logEvent:@"Choice.LocationNag" withParameters:@{ @"Button" : @"Later" }];
    
    NSNumber* timestamp = [NSNumber numberWithDouble:[[NSDate date] timeIntervalSince1970]];
    [NSUserDefaults.standardUserDefaults setObject:timestamp forKey:PREF_LOCATION_NAG_TIMESTAMP];
    [locationPopup hideAnimated];
}

- (void) handleNeverLocationButton
{
    [TMAnalytics logEvent:@"Choice.LocationNag" withParameters:@{ @"Button" : @"Never" }];
    
    [locationPopup hideAnimated];
    [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_SHOW_LOCATION_NAG];
}

#pragma mark -

- (void) gotoAppStore
{
    NSURL *url = [NSURL URLWithString:URL_APPSTORE];
    [[UIApplication sharedApplication] openURL:url];
}

- (BOOL) showRateNagIfNecessary
{
    BOOL isMeasurementJustTaken = [[NSDate date] timeIntervalSince1970] - self.theMeasurement.timestamp < 5.;
    BOOL showRateNag = [[NSUserDefaults.standardUserDefaults objectForKey:PREF_SHOW_RATE_NAG] isEqualToNumber:@YES];
    NSNumber* timeOfLastNag = [NSUserDefaults.standardUserDefaults objectForKey:PREF_RATE_NAG_TIMESTAMP];
    NSTimeInterval secondsSinceLastNag = [[NSDate date] timeIntervalSince1970] - timeOfLastNag.doubleValue;
    BOOL hasBeenNaggedRecently = secondsSinceLastNag < 24 * 60 * 60; // if nagged within the last 24 hours
    
    if (isMeasurementJustTaken && showRateNag && !hasBeenNaggedRecently)
    {
        NSUInteger measurementCount = [TMMeasurement getAllExceptDeleted].count;
        if (measurementCount >= 10)
        {
            __weak TMResultsVC* weakSelf = self;
            dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, 1. * NSEC_PER_SEC);
            dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
                if ([weakSelf.navigationController.viewControllers.lastObject isKindOfClass:[TMResultsVC class]])
                {
            NSNumber* timestamp = [NSNumber numberWithDouble:[[NSDate date] timeIntervalSince1970]];
            [NSUserDefaults.standardUserDefaults setObject:timestamp forKey:PREF_RATE_NAG_TIMESTAMP];
            
                [rateMeView showAnimated];
                }
            });
            
            return YES;
        }
        else
        {
            return NO;
        }
    }
    else
    {
        return NO;
    }
}

- (BOOL) showLocationNagIfNecessary
{
    BOOL isMeasurementJustTaken = [[NSDate date] timeIntervalSince1970] - self.theMeasurement.timestamp < 5.;
    BOOL showLocationNag = [NSUserDefaults.standardUserDefaults boolForKey:PREF_SHOW_LOCATION_NAG];
    NSNumber* timeOfLastNag = [NSUserDefaults.standardUserDefaults objectForKey:PREF_LOCATION_NAG_TIMESTAMP];
    NSTimeInterval secondsSinceLastNag = [[NSDate date] timeIntervalSince1970] - timeOfLastNag.doubleValue;
    BOOL hasBeenNaggedRecently = secondsSinceLastNag < 60 * 60; // if nagged within the last hour
    BOOL alreadyPromptedToTurnOnLocationServices = [NSUserDefaults.standardUserDefaults boolForKey:PREF_PROMPTED_LOCATION_SERVICES];
    BOOL locationServicesEnabled = [CLLocationManager locationServicesEnabled];
    
    if (isMeasurementJustTaken && showLocationNag && !hasBeenNaggedRecently && (locationServicesEnabled || (!locationServicesEnabled && !alreadyPromptedToTurnOnLocationServices)))
    {
        __weak TMResultsVC* weakSelf = self;
        dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, 1. * NSEC_PER_SEC);
        dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
            if ([weakSelf.navigationController.viewControllers.lastObject isKindOfClass:[TMResultsVC class]])
            {
                NSNumber* timestamp = [NSNumber numberWithDouble:[[NSDate date] timeIntervalSince1970]];
                [NSUserDefaults.standardUserDefaults setObject:timestamp forKey:PREF_LOCATION_NAG_TIMESTAMP];
                
                [locationPopup showAnimated];
            }
        });
        
        return YES;
    }
    else
    {
        return NO;
    }
}

- (void) dimScreen
{
    [self.navigationController.view addSubview:dimmingView];
    [dimmingView addMatchSuperviewConstraints];
}

- (void) unDimScreen
{
    [dimmingView removeConstraintsFromSuperview];
    [dimmingView removeFromSuperview];
}

@end
