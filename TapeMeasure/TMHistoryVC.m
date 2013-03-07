//
//  TMHistoryVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMHistoryVC.h"

@implementation TMHistoryVC

MBProgressHUD *HUD;

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
     NSLog(@"viewDidLoad: TMHistoryVC");
    [super viewDidLoad];
    
    [self refreshPrefs];
    
    [self loginOrCreateAccount:^{
        [self syncWithServer:^{
            [self refreshTableView];
        }];
    }];
}

- (void)viewDidAppear:(BOOL)animated
{
    NSLog(@"viewDidAppear: TMHistoryVC");
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
        
        UIBarButtonItem *backBtn = [[UIBarButtonItem alloc] initWithTitle:@"History" style:UIBarButtonItemStyleBordered target:nil action:nil];
        self.navigationItem.backBarButtonItem = backBtn;
    }
    else if([[segue identifier] isEqualToString:@"toType"])
    {
        UIBarButtonItem *backBtn = [[UIBarButtonItem alloc] initWithTitle:@"Cancel" style:UIBarButtonItemStyleBordered target:nil action:nil];
        self.navigationItem.backBarButtonItem = backBtn;
    }
}

#pragma mark - Server stuff

- (void) loginOrCreateAccount:(void (^)())completionBlock
{
    if ([USER_MANAGER isLoggedIn])
    {
        if (completionBlock) completionBlock();
    }
    else
    {
        if ([USER_MANAGER hasValidStoredCredentials])
        {
            [self login:completionBlock];
        }
        else
        {
            [self createAnonAccount:^(){
                 [self login:completionBlock];
             }];
        }
    }
}

- (void)createAnonAccount:(void (^)())completionBlock
{
    [USER_MANAGER
     createAnonAccount:^(NSString* username)
     {
         [Flurry logEvent:@"User.AnonAccountCreated"];
         if (completionBlock) completionBlock();
     }
     onFailure:^(int statusCode)
     {
         NSLog(@"createAnonAccount failure callback:%i", statusCode);
         [Flurry
          logError:@"HTTP.CreateAnonAccount"
          message:[NSString stringWithFormat:@"%i", statusCode]
          error:nil
          ];
     }
     ];
}

- (void) login:(void (^)())completionBlock
{
    [USER_MANAGER
     loginWithStoredCredentials:^()
     {
         NSLog(@"Login success callback");
         if (completionBlock) completionBlock();
     }
     onFailure:^(int statusCode)
     {
         NSLog(@"Login failure callback:%i", statusCode);
         [Flurry
          logError:@"HTTP.Login"
          message:[NSString stringWithFormat:@"%i", statusCode]
          error:nil
          ];
     }
     ];
}

- (void)syncWithServer:(void (^)())completionBlock
{
    int lastTransId = [[NSUserDefaults standardUserDefaults] integerForKey:PREF_LAST_TRANS_ID];
    
    [TMLocation
     syncWithServer:lastTransId
     onSuccess:^(int transId)
     {
         [TMLocation saveLastTransIdIfHigher:transId];
         
         [TMMeasurement
          syncWithServer:lastTransId
          onSuccess:^(int transId)
          {
              [TMMeasurement saveLastTransIdIfHigher:transId];
              [TMMeasurement associateWithLocations];
              if (completionBlock) completionBlock();
          }
          onFailure:nil]; //TODO: handle failure
     }
     onFailure:nil]; //TODO: handle failure
    
    //TODO: sync in parallel?
}

- (void)logout:(void (^)())completionBlock
{
    NSLog(@"Logging out...");
    
    HUD = [[MBProgressHUD alloc] initWithView:self.navigationController.view];
	[self.navigationController.view addSubview:HUD];
	HUD.labelText = @"Thinking..";
	[HUD show:YES];
    
    [Flurry logEvent:@"User.Logout"];
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        
        [USER_MANAGER logout];
        [TMMeasurement deleteAllFromDb];
        [[NSUserDefaults standardUserDefaults] setObject:nil forKey:PREF_LAST_TRANS_ID];
        
        dispatch_async(dispatch_get_main_queue(), ^{
            if (completionBlock) completionBlock();
        });
    });
}

- (void)handleLogoutDone
{
    [self refreshTableView];
    [self createAnonAccount:^(){
         [self login:^{
             if (HUD) [HUD hide:YES];
         }];
     }];
}

#pragma mark - Private methods

/** Expensive. Can cause UI to lag if called at the wrong time. */
- (void)setupDataCapture
{
    [RCAVSessionManagerFactory setupAVSession];
    [RCVideoCapManagerFactory setupVideoCapWithSession:[SESSION_MANAGER session]];
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
    
    [Flurry logEvent:@"Measurement.Delete.History"];
    
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
    
    TMMeasurement *measurement = [measurementsData objectAtIndex:indexPath.row];
    
    if (measurement.name.length == 0) {
        cell.textLabel.text = [NSDateFormatter localizedStringFromDate:[NSDate dateWithTimeIntervalSince1970:measurement.timestamp]
                                                             dateStyle:NSDateFormatterShortStyle timeStyle:NSDateFormatterShortStyle];
    } else {
        cell.textLabel.text = measurement.name;
    }
   
    switch (measurement.type)
    {
        case TypeTotalPath:
            cell.detailTextLabel.text = [measurement getFormattedDistance:measurement.totalPath];
            break;
            
        case TypeHorizontal:
            cell.detailTextLabel.text = [measurement getFormattedDistance:measurement.horzDist];
            break;
            
        case TypeVertical:
            cell.detailTextLabel.text = [measurement getFormattedDistance:measurement.vertDist];
            break;
            
        default: //TypePointToPoint
            cell.detailTextLabel.text = [measurement getFormattedDistance:measurement.pointToPoint];
            break;
    }
    
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
    
    if ([USER_MANAGER isLoggedIn] && ![USER_MANAGER isUsingAnonAccount])
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
            if ([USER_MANAGER isLoggedIn] && ![USER_MANAGER isUsingAnonAccount])
            {
                [self logout:^{
                    [self handleLogoutDone];
                }];
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
            [self syncWithServer:^{
                [self refreshTableView];
            }];
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
