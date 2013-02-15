//
//  TMHistoryVC.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMHistoryVC.h"

@interface TMHistoryVC ()

@end

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
}

- (void)viewDidAppear:(BOOL)animated
{
    NSLog(@"viewDidAppear");
    [self loadTableData];
    [self.tableView reloadData];
    
    [self performSelectorInBackground:@selector(setupDataCapture) withObject:nil];
    
    //must run on UI thread for some reason
    [LOCATION_MANAGER startLocationUpdates]; //TODO: prevent unnecessary updates
    
    if ([USER_MANAGER isLoggedIn])
    {
        [self syncWithServer];
    }
    else
    {
        [USER_MANAGER fetchSessionCookie:^{ [self login]; } onFailure:nil]; //TODO: handle failure
    }
}

- (void)login
{
    [USER_MANAGER loginWithUsername:@"ben"
                       withPassword:@"ben"
                          onSuccess:^()
                                     {
                                         NSLog(@"Login success callback");
                                         [self syncWithServer];
                                     }
                          onFailure:^(int statusCode)
                                     {
                                         NSLog(@"Login failure callback");
                                         //TODO: handle failure
                                     }
     ];
}

/** Expensive. Can cause UI to lag if called at the wrong time. */
- (void)setupDataCapture
{
    [RCAVSessionManagerFactory setupAVSession];
    [RCVideoCapManagerFactory setupVideoCapWithSession:[SESSION_MANAGER session]];
}

- (void)viewWillAppear:(BOOL)animated
{
    [self.navigationController setToolbarHidden:YES animated:animated];
    [super viewWillAppear:animated];
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

- (void)didDismissModalView
{
    NSLog(@"didDismissModalView");
    [self refreshPrefs];
    [self.tableView reloadData];
}

- (void)viewDidUnload {
    [self setActionButton:nil];
    [super viewDidUnload];
}

#pragma mark - Private methods

- (void)refreshPrefs
{
    unitsPref = [[NSUserDefaults standardUserDefaults] objectForKey:@"Units"];
    fractionalPref = [[NSUserDefaults standardUserDefaults] objectForKey:@"Fractional"];
}

- (void)loadTableData
{
    NSLog(@"loadTableData");
    
    measurementsData = [TMMeasurement getAllMeasurementsExceptDeleted];
}

- (void)deleteMeasurement:(NSIndexPath*)indexPath
{
    [self.tableView beginUpdates];
     
    TMMeasurement *theMeasurement = [measurementsData objectAtIndex:indexPath.row];
    theMeasurement.deleted = YES;
    theMeasurement.syncPending = YES;
    [DATA_MANAGER saveContext];
    
    [self.tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
    [self loadTableData];
    [self.tableView endUpdates];
    
    [theMeasurement
     putMeasurement:^(int transId) {
         NSLog(@"putMeasurement success callback");
         [theMeasurement deleteMeasurement];
         [DATA_MANAGER saveContext];
     }
     onFailure:^(int statusCode) {
         NSLog(@"putMeasurement failure callback");
     }
    ];
}

- (void)syncWithServer
{
    [TMMeasurement
     syncMeasurements:
     ^(int transCount)
     {
         [self loadTableData];
         [self.tableView reloadData];
     }
     onFailure:nil]; //TODO: handle failure
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
    actionSheet = [[UIActionSheet alloc] initWithTitle:@"Menu"
                                              delegate:self
                                     cancelButtonTitle:@"Cancel"
                                destructiveButtonTitle:nil
                                     otherButtonTitles:@"Create Account", @"Share app with a friend", @"Refresh List", @"About", nil];
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
            NSLog(@"Create Account");
            [self performSegueWithIdentifier:@"toCreateAccount" sender:self];
            break;
        }
        case 1:
        {
            NSLog(@"Share");
            break;
        }
        case 2:
        {
            NSLog(@"Refresh");
            [self syncWithServer];
            break;
        }
        case 3:
        {
            NSLog(@"About");
            break;
        }
        default:
            break;
    }
}
@end
