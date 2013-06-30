//
//  TMHistoryVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMHistoryVC.h"

@implementation TMHistoryVC

#pragma mark - Event handlers

- (id)initWithStyle:(UITableViewStyle)style
{
    self = [super initWithStyle:style];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self refreshPrefs];
    [self refreshTableView];
    
    //must execute on UI thread
    if ([LOCATION_MANAGER isLocationAuthorized])
    {
        [LOCATION_MANAGER startLocationUpdates];
    }
    else if([self shouldShowLocationExplanation])
    {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Location"
                                                        message:@"If you allow the app to use your location, we can improve the accuracy of your measurements by adjusting for altitude and how far you are from the equator. If you don't want us to save your location data after the measurement, you can turn that off in the settings."
                                                       delegate:self
                                              cancelButtonTitle:@"Continue"
                                              otherButtonTitles:nil];
        alert.tag = AlertLocation;
        [alert show];
    }
    
    __weak TMHistoryVC* weakSelf = self;
    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^
    {
        [RCHTTPClient initWithBaseUrl:API_BASE_URL withAcceptHeader:API_HEADER_ACCEPT withApiVersion:API_VERSION];
        [weakSelf loginOrCreateAnonAccount];
    });
}

- (void)viewDidAppear:(BOOL)animated
{
    [TMAnalytics logEvent:@"View.History"];
    [self refreshTableView];
    [self performSelectorInBackground:@selector(setupDataCapture) withObject:nil];
}

- (void)viewWillAppear:(BOOL)animated
{
    [self.navigationController setToolbarHidden:YES animated:animated];
    [super viewWillAppear:animated];
}

- (void)viewDidUnload {
    [self setActionButton:nil];
    [super viewDidUnload];
}

- (void)didDismissModalView
{
    NSLog(@"didDismissModalView");
    [self refreshPrefs];
    [self.tableView reloadData];
}

- (void) prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if ([[segue identifier] isEqualToString:@"toResult"])
    {
        NSIndexPath *indexPath = (NSIndexPath*)sender;
        TMMeasurement *measurement = [measurementsData objectAtIndex:indexPath.row];
        
        TMResultsVC *resultsVC = [segue destinationViewController];
        resultsVC.theMeasurement = measurement;
    }
}

#pragma mark - Private methods

- (BOOL)shouldShowLocationExplanation
{
    if ([CLLocationManager locationServicesEnabled])
    {
        return [CLLocationManager authorizationStatus] == kCLAuthorizationStatusNotDetermined;
    }
    else
    {
        return [[NSUserDefaults standardUserDefaults] boolForKey:PREF_SHOW_LOCATION_EXPLANATION];
    }
}

- (void) loginOrCreateAnonAccount
{
    if ([USER_MANAGER getLoginState] == LoginStateYes)
    {
        [self syncWithServer];
    }
    else
    {
        if ([USER_MANAGER hasValidStoredCredentials])
        {
            [self login];
        }
        else
        {
            __weak TMHistoryVC* weakSelf = self;
            [SERVER_OPS
             createAnonAccount: ^{
                 [weakSelf login];
             }
             onFailure: ^{
                 //fail silently. will try again next time app is started.
             }];
        }
    }
}

- (void) login
{
    __weak TMHistoryVC* weakSelf = self;
    [SERVER_OPS
     login: ^{
         [weakSelf syncWithServer];
     }
     onFailure: ^(int statusCode){
         if (![USER_MANAGER isUsingAnonAccount] && statusCode == 200) //we get 200 on wrong user/pass
         {
             UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Whoops"
                                                             message:@"Failed to login. Press OK to enter your login details again."
                                                            delegate:self
                                                   cancelButtonTitle:@"Not now"
                                                   otherButtonTitles:@"OK", nil];
             alert.tag = AlertLoginFailure;
             [alert show];
         }
         else if (![USER_MANAGER hasValidStoredCredentials])
         {
             [RCUser deleteStoredUser]; //will create new anon acct next time app is launched
         }
     }];
}

- (void) alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
{
    if (alertView.tag == AlertLoginFailure)
    {
        if (buttonIndex == 1) [self performSegueWithIdentifier:@"toLogin" sender:self];
    }
    if (alertView.tag == AlertLocation)
    {
        if (buttonIndex == 0) //the only button
        {
            [[NSUserDefaults standardUserDefaults] setBool:NO forKey:PREF_SHOW_LOCATION_EXPLANATION];
            [[NSUserDefaults standardUserDefaults] synchronize];
            if([LOCATION_MANAGER shouldAttemptLocationAuthorization]) [LOCATION_MANAGER startLocationUpdates]; //only attempt to authorize/update location if location is globally enabled
        }
    }
}

- (void) syncWithServer
{
    __weak TMHistoryVC* weakSelf = self;
    [SERVER_OPS
     syncWithServer: ^(BOOL updated){
         updated ? [weakSelf refreshTableViewWithProgress] : [weakSelf refreshTableView];
     }
     onFailure: ^{
         NSLog(@"Sync failure callback");
     }];
}

- (void) logout
{
    HUD = [[MBProgressHUD alloc] initWithView:self.navigationController.view];
    [self.navigationController.view addSubview:HUD];
    HUD.labelText = @"Thinking";
    [HUD show:YES];
    
    __weak TMHistoryVC* weakSelf = self;
    [SERVER_OPS logout:^{
        [weakSelf handleLogoutDone];
    }];
}

- (void) handleLogoutDone
{
    [self refreshTableView];
    
    [SERVER_OPS createAnonAccount:^{
        [SERVER_OPS login:^{
            [HUD hide:YES];
        }
        onFailure:^(int statusCode){
            NSLog(@"Login failure callback");
            [HUD hide:YES];
        }];
    }
    onFailure:^{
        NSLog(@"Create anon account failure callback");
        [HUD hide:YES];
    }];
}

- (void)setupDataCapture
{
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        /** Expensive. Can cause UI to lag if called at the wrong time. */
        [[RCVideoManager sharedInstance] setupWithSession:SESSION_MANAGER.session];
        [RCMotionManager sharedInstance]; // inits the singleton now so that it doesn't slow us down later.
    });
}

- (void)refreshPrefs
{
    unitsPref = [[NSUserDefaults standardUserDefaults] objectForKey:@"Units"];
    fractionalPref = [[NSUserDefaults standardUserDefaults] objectForKey:@"Fractional"];
}

- (void)loadTableData
{
    NSLog(@"loadTableData");
    
    measurementsData = [TMMeasurement getAllExceptDeleted];
}

- (void)refreshTableViewWithProgress
{
    HUD = [[MBProgressHUD alloc] initWithView:self.navigationController.view];
    [self.navigationController.view addSubview:HUD];
    HUD.labelText = @"Updating";
    [HUD show:YES];
    
    //delay slightly so progress spinner can appear
    __weak TMHistoryVC* weakSelf = self;
    dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, 0.01 * NSEC_PER_SEC);
    dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
        [weakSelf refreshTableView];
        [NSThread sleepForTimeInterval:1]; //introduce an artificial pause while list is updating so user doesn't accidentally press the wrong thing
        [HUD hide:YES];
    });
}

- (void)refreshTableView
{
    [self loadTableData];
    [self.tableView reloadData];
}

- (void)deleteMeasurement:(NSIndexPath*)indexPath
{
    [self.tableView beginUpdates];
     
    TMMeasurement *theMeasurement = [measurementsData objectAtIndex:indexPath.row];
    theMeasurement.deleted = YES;
    theMeasurement.syncPending = YES;
    [DATA_MANAGER saveContext];
    
    [TMAnalytics logEvent:@"Measurement.Delete.History"];
    
    [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
    [self loadTableData];
    [self.tableView endUpdates];
    
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
    return measurementsData.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *CellIdentifier = @"Cell";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    UILabel* nameLabel = (UILabel*)[cell viewWithTag:1];
    RCDistanceLabel* valueLabel = (RCDistanceLabel*)[cell viewWithTag:2];
    
    TMMeasurement *measurement = [measurementsData objectAtIndex:indexPath.row];
    
    if (measurement.name.length == 0) {
        nameLabel.text = [NSDateFormatter localizedStringFromDate:[NSDate dateWithTimeIntervalSince1970:measurement.timestamp]
                                                             dateStyle:NSDateFormatterShortStyle timeStyle:NSDateFormatterShortStyle];
    } else {
        nameLabel.text = measurement.name;
    }
    
    [valueLabel setDistance:[measurement getPrimaryDistanceObject]];
    
    return cell;
}


// Override to support conditional editing of the table view.
//- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
//{
//    // Return NO if you do not want the specified item to be editable.
//    return YES;
//}


// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (editingStyle == UITableViewCellEditingStyleDelete)
    {
        [self deleteMeasurement:indexPath];
    }   
}

/*
// Override to support rearranging the table view.
- (void)tableView:(UITableView *)tableView moveRowAtIndexPath:(NSIndexPath *)fromIndexPath toIndexPath:(NSIndexPath *)toIndexPath
{
}
*/

/*
// Override to support conditional rearranging of the table view.
- (BOOL)tableView:(UITableView *)tableView canMoveRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the item to be re-orderable.
    return YES;
}
*/

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [self performSegueWithIdentifier:@"toResult" sender:indexPath];
}

#pragma mark - Action sheet

- (IBAction)handleActionButton:(id)sender {
    [self showActionSheet];
}

- (void)showActionSheet
{
    NSString *firstButtonTitle;
    
    if ([USER_MANAGER getLoginState] != LoginStateNo && ![USER_MANAGER isUsingAnonAccount])
    {
        firstButtonTitle = @"Logout";
    }
    else
    {
        firstButtonTitle = @"Create Account or Login";
    }
    
    actionSheet = [[UIActionSheet alloc] initWithTitle:@"Menu"
                                              delegate:self
                                     cancelButtonTitle:@"Cancel"
                                destructiveButtonTitle:nil
                                     otherButtonTitles:firstButtonTitle, @"Tell a friend", @"Refresh List", @"About", nil];
    // Show the sheet
    [actionSheet showFromBarButtonItem:_actionButton animated:YES];
}

- (void)actionSheet:(UIActionSheet *)actionSheet didDismissWithButtonIndex:(NSInteger)buttonIndex
{
    NSLog(@"Button %d", buttonIndex);
    
    switch (buttonIndex)
    {
        case 0:
        {
            NSLog(@"Account button");
            if ([USER_MANAGER getLoginState] != LoginStateNo && ![USER_MANAGER isUsingAnonAccount])
            {
                [self logout];
            }
            else
            {
                [self performSegueWithIdentifier:@"toCreateAccount" sender:self];
            }
            break;
        }
        case 1:
        {
            NSLog(@"Share button");
            break;
        }
        case 2:
        {
            NSLog(@"Refresh button");
            [self syncWithServer];
            break;
        }
        case 3:
        {
            NSLog(@"About button");
            break;
        }
        default:
            break;
    }
}
@end
