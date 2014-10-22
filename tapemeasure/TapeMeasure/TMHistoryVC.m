//
//  TMHistoryVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMHistoryVC.h"
#import "UIImage+InverseImage.h"
#import "CustomIOS7AlertView.h"
#import "TMAboutView.h"
#import "TMTipsView.h"
#import "TMLocalMoviePlayer.h"
#import "TMPreferences.h"
#import <RCCore/RCCore.h>

@implementation TMHistoryVC
{
    CustomIOS7AlertView *aboutView;
    CustomIOS7AlertView *tipsView;
    CustomIOS7AlertView *unitsView;
    TMShareSheet* shareSheet;
    RCModalCoverVerticalTransition* transition;
}

#pragma mark - Event handlers

- (BOOL)hidesBottomBarWhenPushed { return YES; }

- (void)viewDidLoad
{
    [super viewDidLoad];
    [self refreshPrefs];
    [self refreshTableView];
    
    // make menu button white on iOS < 7
    if (SYSTEM_VERSION_LESS_THAN(@"7.0"))
    {
        self.actionButton.image = [self.actionButton.image invertedImage];
    }
    
    aboutView = [[CustomIOS7AlertView alloc] init];
    aboutView.layer.backgroundColor = [[UIColor whiteColor] CGColor];
    [aboutView setContainerView:[[TMAboutView alloc] initWithFrame:CGRectMake(0, 0, 290, 210)]];
    [aboutView setButtonTitles:[NSArray arrayWithObject:@"Close"]];
    [aboutView setOnButtonTouchUpInside:^(CustomIOS7AlertView *alertView_, int buttonIndex) {
        [alertView_ close];
    }];
    [aboutView setOnTouchUpOutside:^(CustomIOS7AlertView *alertView_) {
        [alertView_ close];
    }];
    
    int tipsViewWidth = UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad ? 400 : 300;
    int tipsViewHeight = UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad ? 540 : 350;
    tipsView = [[CustomIOS7AlertView alloc] init];
    tipsView.layer.backgroundColor = [[UIColor whiteColor] CGColor];
    [tipsView setContainerView:[[TMTipsView alloc] initWithFrame:CGRectMake(0, 0, tipsViewWidth, tipsViewHeight)]];
    [tipsView setButtonTitles:[NSArray arrayWithObject:@"Close"]];
    [tipsView setOnButtonTouchUpInside:^(CustomIOS7AlertView *tipsView_, int buttonIndex) {
        [tipsView_ close];
    }];
    [tipsView setOnTouchUpOutside:^(CustomIOS7AlertView *tipsView_) {
        [tipsView_ close];
    }];
    
//    __weak TMHistoryVC* weakSelf = self;
//    dispatch_async(dispatch_get_global_queue( DISPATCH_QUEUE_PRIORITY_LOW, 0), ^
//    {
//        [RCHTTPClient initWithBaseUrl:API_BASE_URL withAcceptHeader:API_HEADER_ACCEPT withApiVersion:API_VERSION];
//        [weakSelf loginIfPossible];
//    });
}

- (void)viewWillAppear:(BOOL)animated
{
    [self.navigationController setToolbarHidden:YES animated:NO]; // workaround for iOS bug where toolbar will appear when popping back to this view controller
}

- (void)viewDidAppear:(BOOL)animated
{
    [TMAnalytics logEvent:@"View.History"];
    [self refreshTableView];
    
    self.navigationItem.title = self.title; // workaround for title not appearing when VC not presented with an animation
}

- (void)viewDidUnload {
    [self setActionButton:nil];
    [super viewDidUnload];
}

- (void)didDismissModalView
{
    LOGME
    [self refreshPrefs];
    [self.tableView reloadData];
}

- (void) gotoResult:(NSIndexPath*)indexPath
{
    TMMeasurement *measurement = measurementsData[indexPath.row];
    TMResultsVC *resultsVC;
    
    if ([self.navigationController.secondToLastViewController isKindOfClass:[TMResultsVC class]])
    {
        resultsVC = (TMResultsVC*)self.navigationController.secondToLastViewController;
        resultsVC.theMeasurement = measurement;
        [self.navigationController popViewControllerAnimated:YES];
    }
    else
    {
        resultsVC = [self.navigationController.storyboard instantiateViewControllerWithIdentifier:@"Results"];
        resultsVC.theMeasurement = measurement;
        [self.navigationController pushViewController:resultsVC animated:YES];
    }
}

#pragma mark - Private methods

- (void) gotoTutorialVideo
{
    TMLocalMoviePlayer* movieViewController = [self.storyboard instantiateViewControllerWithIdentifier:@"Tutorial"];
    [self presentViewController:movieViewController animated:YES completion:nil];
}

- (void) moviePlayerWillExitFullScreen:(NSNotification*)notification
{
    [self dismissMoviePlayerViewControllerAnimated];
}

//- (void) loginIfPossible
//{
//    if ([USER_MANAGER getLoginState] == LoginStateYes)
//    {
//        [self syncWithServer];
//    }
//    else
//    {
//        if ([USER_MANAGER hasValidStoredCredentials]) [self login];
//    }
//}
//
//- (void) login
//{
//    __weak TMHistoryVC* weakSelf = self;
//    [SERVER_OPS
//     login: ^{
//         [weakSelf syncWithServer];
//     }
//     onFailure: ^(NSInteger statusCode){
//         if (![USER_MANAGER isUsingAnonAccount] && statusCode == 401) //we get 401 on wrong user/pass
//         {
//             UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Whoops"
//                                                             message:@"Failed to login. Press OK to enter your login details again."
//                                                            delegate:self
//                                                   cancelButtonTitle:@"Not now"
//                                                   otherButtonTitles:@"OK", nil];
//             alert.tag = AlertLoginFailure;
//             [alert show];
//         }
//         else if (![USER_MANAGER hasValidStoredCredentials])
//         {
//             [RCUser deleteStoredUser]; //will create new anon acct next time app is launched
//         }
//     }];
//}
//
//- (void) alertView:(UIAlertView *)alertView clickedButtonAtIndex:(NSInteger)buttonIndex
//{
//    if (alertView.tag == AlertLoginFailure)
//    {
//        if (buttonIndex == 1) [self performSegueWithIdentifier:@"toLogin" sender:self];
//    }
//}
//
//- (void) syncWithServer
//{
//    __weak TMHistoryVC* weakSelf = self;
//    [SERVER_OPS
//     syncWithServer: ^(BOOL updated){
//         updated ? [weakSelf refreshTableViewWithProgress] : [weakSelf refreshTableView];
//     }
//     onFailure: ^{
//         DLog(@"Sync failure callback");
//     }];
//}
//
//- (void) logout
//{
//    HUD = [[MBProgressHUD alloc] initWithView:self.navigationController.view];
//    [self.navigationController.view addSubview:HUD];
//    HUD.labelText = @"Thinking";
//    [HUD show:YES];
//    
//    __weak TMHistoryVC* weakSelf = self;
//    [SERVER_OPS logout:^{
//        [weakSelf handleLogoutDone];
//    }];
//}
//
//- (void) handleLogoutDone
//{
//    [self refreshTableView];
//    
//    [SERVER_OPS createAnonAccount:^{
//        [SERVER_OPS login:^{
//            [HUD hide:YES];
//        }
//        onFailure:^(NSInteger statusCode){
//            DLog(@"Login failure callback");
//            [HUD hide:YES];
//        }];
//    }
//    onFailure:^{
//        DLog(@"Create anon account failure callback");
//        [HUD hide:YES];
//    }];
//}

- (void)refreshPrefs
{
    unitsPref = [[NSUserDefaults standardUserDefaults] objectForKey:@"Units"];
    fractionalPref = [[NSUserDefaults standardUserDefaults] objectForKey:@"Fractional"];
}

- (void)loadTableData
{
    LOGME
    measurementsData = [TMMeasurement getAllExceptDeleted];
}

//- (void)refreshTableViewWithProgress
//{
//    HUD = [[MBProgressHUD alloc] initWithView:self.navigationController.view];
//    [self.navigationController.view addSubview:HUD];
//    HUD.labelText = @"Updating";
//    [HUD show:YES];
//    
//    //delay slightly so progress spinner can appear
//    __weak TMHistoryVC* weakSelf = self;
//    dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, 0.01 * NSEC_PER_SEC);
//    dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
//        [weakSelf refreshTableView];
//        [NSThread sleepForTimeInterval:1]; //introduce an artificial pause while list is updating so user doesn't accidentally press the wrong thing
//        [HUD hide:YES];
//    });
//}

- (void)refreshTableView
{
    [self loadTableData];
    [self.tableView reloadData];
}

- (void)deleteMeasurement:(NSIndexPath*)indexPath
{
    [self.tableView beginUpdates];
     
    TMMeasurement *theMeasurement = measurementsData[indexPath.row];
    theMeasurement.deleted = YES;
    theMeasurement.syncPending = YES;
    [DATA_MANAGER saveContext];
    
    [TMAnalytics logEvent:@"Measurement.Delete.History"];
    
    [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationAutomatic];
    [self loadTableData];
    [self.tableView endUpdates];
    
//    [theMeasurement
//     putToServer:^(NSInteger transId) {
//         DLog(@"putMeasurement success callback");
//         [theMeasurement deleteFromDb];
//         [DATA_MANAGER saveContext];
//     }
//     onFailure:^(NSInteger statusCode) {
//         DLog(@"putMeasurement failure callback");
//     }
//    ];
}

- (void) linkTapped:(id)sender
{
    NSURL *url = [NSURL URLWithString:URL_WEBSITE];
    [[UIApplication sharedApplication] openURL:url];
}

- (IBAction)handleNewButton:(id)sender
{
    [self.navigationController popToRootViewControllerAnimated:NO];
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
    valueLabel.textAlignment = NSTextAlignmentRight;
    
    TMMeasurement *measurement = measurementsData[indexPath.row];
    
    if (measurement.name.length == 0) {
        nameLabel.text = [NSDateFormatter localizedStringFromDate:[NSDate dateWithTimeIntervalSince1970:measurement.timestamp]
                                                             dateStyle:NSDateFormatterShortStyle timeStyle:NSDateFormatterShortStyle];
    } else {
        nameLabel.text = measurement.name;
    }
    
    [valueLabel setDistance:[measurement getPrimaryDistanceObject]];
    
    return cell;
}

- (void) tableView:(UITableView *)tableView willBeginEditingRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell* cell = [tableView cellForRowAtIndexPath:indexPath];
    RCDistanceLabel* valueLabel = (RCDistanceLabel*)[cell viewWithTag:2];
    valueLabel.hidden = YES; // workaround for broken delete button in iOS 7
}

- (void)tableView:(UITableView *)tableView didEndEditingRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell* cell = [tableView cellForRowAtIndexPath:indexPath];
    RCDistanceLabel* valueLabel = (RCDistanceLabel*)[cell viewWithTag:2];
    valueLabel.hidden = NO;
}

// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (editingStyle == UITableViewCellEditingStyleDelete)
    {
        UITableViewCell* cell = [tableView cellForRowAtIndexPath:indexPath];
        cell.hidden = YES; // workaround for broken delete animation in iOS 7
        [self deleteMeasurement:indexPath];
    }   
}

// following two methods solve a layout warning in iOS 8 http://stackoverflow.com/questions/25822324/ios8-constraints-ambiguously-suggest-a-height-of-zero
- (CGFloat)tableView:(UITableView *)tableView estimatedHeightForRowAtIndexPath:(NSIndexPath *)indexPath
{
    return 44;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
    return 44;
}

#pragma mark - Table view delegate

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [self gotoResult:indexPath];
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)aTableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath
{
    return UITableViewCellEditingStyleDelete;
}

#pragma mark - Action sheet

- (IBAction)handleActionButton:(id)sender
{
    if (actionSheet.isVisible)
        [self dismissActionSheet];
    else
        [self showActionSheet];
}

- (void)showActionSheet
{
    [TMAnalytics logEvent:@"View.History.Menu"];
    
    if (actionSheet == nil)
    {
        actionSheet = [[UIActionSheet alloc] initWithTitle:@"Endless Tape Measure Menu"
                                                  delegate:self
                                         cancelButtonTitle:@"Cancel"
                                    destructiveButtonTitle:nil
                                         otherButtonTitles:@"About", @"Share app", @"Rate the app", @"Accuracy tips", @"Tutorial video", @"Preferences", nil];
    }
    
    // Show the sheet
    [actionSheet showFromBarButtonItem:_actionButton animated:YES];
}

- (void) dismissActionSheet
{
    [actionSheet dismissWithClickedButtonIndex:-1 animated:NO];
}

- (void)actionSheet:(UIActionSheet *)actionSheet didDismissWithButtonIndex:(NSInteger)buttonIndex
{
    switch (buttonIndex)
    {
        case 0:
        {
            [TMAnalytics logEvent:@"View.About"];
            [aboutView show];
            break;
        }
        case 1:
        {
            [TMAnalytics logEvent:@"View.ShareApp"];
            [self showShareSheet];
            break;
        }
        case 2:
        {
            [TMAnalytics logEvent:@"View.Rate"];
            [self gotoAppStore];
            break;
        }
        case 3:
        {
            [TMAnalytics logEvent:@"View.Tips"];
            [tipsView show];
            break;
        }
        case 4:
        {
            [self gotoTutorialVideo];
            break;
        }
        case 5:
        {
            [self gotoPreferences];
            break;
        }

        default:
            break;
    }
}

- (void) gotoPreferences
{
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
        [self performSegueWithIdentifier:@"toSettings" sender:self];
    }
    else
    {
        TMPreferences* prefsController = [self.storyboard instantiateViewControllerWithIdentifier:@"Preferences"];
        if (transition == nil) transition = [RCModalCoverVerticalTransition new];
        prefsController.transitioningDelegate = transition;
        prefsController.modalPresentationStyle = UIModalPresentationCustom;
        [self presentViewController:prefsController animated:YES completion:nil];
    }
}

- (void) gotoAppStore
{
    NSURL *url = [NSURL URLWithString:URL_APPSTORE];
    [[UIApplication sharedApplication] openURL:url];
}

#pragma mark - Sharing

- (NSString*) composeSharingString
{
    NSString* result = [NSString stringWithFormat: @"Check out this app that uses computer vision to take accurate measurements with your iOS device. %@", URL_SHARING];
    return result;
}

- (void) showShareSheet
{
    OSKShareableContent *content = [OSKShareableContent contentFromText:[self composeSharingString]];
    content.title = @"Share App";
    shareSheet = [TMShareSheet shareSheetWithDelegate:self];
    [OSKActivitiesManager sharedInstance].customizationsDelegate = shareSheet;
    [shareSheet showShareSheet_Pad_FromBarButtonItem:self.actionButton content:content];
}

#pragma mark - TMShareSheetDelegate

- (OSKActivityCompletionHandler) activityCompletionHandler
{
    OSKActivityCompletionHandler activityCompletionHandler = ^(OSKActivity *activity, BOOL successful, NSError *error){
        if (successful) {
            [TMAnalytics logEvent:@"Share.App" withParameters:@{ @"Type": [activity.class activityName] }];
        } else {
            [TMAnalytics logError:@"Share.App" message:[activity.class activityName] error:error];
        }
    };
    return activityCompletionHandler;
}

@end
