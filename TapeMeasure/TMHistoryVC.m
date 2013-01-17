//
//  TMMeasurementsList.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMHistoryVC.h"
#import "TMMeasurement.h"
#import "TMAppDelegate.h"
#import "TMResultsVC.h"
#import "TMDistanceFormatter.h"
#import "TMAppDelegate.h"
#import "TMAvSessionManagerFactory.h"

@interface TMHistoryVC ()

@end

@implementation TMHistoryVC

@synthesize measurementsData;

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
    
    //register to receive notifications of pause/resume events
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];

    
    [self refreshPrefs];
    
    appDel = (TMAppDelegate*)[[UIApplication sharedApplication] delegate];
}

- (void)viewDidAppear:(BOOL)animated
{
    NSLog(@"viewDidAppear");
    [self loadTableData];
    [self.tableView reloadData];
    
    
    [appDel setupDataCapture];
    [appDel startLocationUpdates]; //TODO: prevent unnecessary updates
}

- (void)viewWillAppear:(BOOL)animated
{
    [self.navigationController setToolbarHidden:YES animated:animated];
    [super viewWillAppear:animated];
}

//- (void)viewWillDisappear:(BOOL)animated
//{
//    [self.navigationController setToolbarHidden:NO animated:animated];
//    [super viewWillDisappear:animated];
//}

- (void)handleResume
{
//    [self loadTableData];
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
    [super viewDidUnload];
}

- (IBAction)handleDeleteButton:(id)sender
{
    //    NSArray *deleteIndexPaths = [NSArray arrayWithObjects:
    //                                 [NSIndexPath indexPathForRow:1 inSection:0],
    //                                 nil];
    //
    //    UITableView *tv = (UITableView *)self.view;
    //
    //    [tv beginUpdates];
    //    [tv deleteRowsAtIndexPaths:deleteIndexPaths withRowAnimation:UITableViewRowAnimationFade];
    //    [tv endUpdates];
    //
    //    [self.tableView reloadData];
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
    
    TMAppDelegate* appDel = (TMAppDelegate*)[[UIApplication sharedApplication] delegate];
    _managedObjectContext = [appDel managedObjectContext];
    
    NSFetchRequest *fetchRequest = [[NSFetchRequest alloc] init];
    NSEntityDescription *entity = [NSEntityDescription entityForName:ENTITY_MEASUREMENT inManagedObjectContext:_managedObjectContext];
    
    NSSortDescriptor *sortDescriptor = [[NSSortDescriptor alloc]
                                        initWithKey:@"timestamp"
                                        ascending:NO];
    
    NSArray *descriptors = [[NSArray alloc] initWithObjects:sortDescriptor, nil];
    
    [fetchRequest setSortDescriptors:descriptors];
    [fetchRequest setEntity:entity];
    
    NSError *error;
    self.measurementsData = [_managedObjectContext executeFetchRequest:fetchRequest error:&error]; //TODO: Handle fetch error
    
    if(error)
    {
        NSLog(@"Error loading table data: %@", [error localizedDescription]);
    }
}

- (void)deleteMeasurement:(NSIndexPath*)indexPath
{
    NSError *error;
    TMMeasurement *theMeasurement = [measurementsData objectAtIndex:indexPath.row];
    [theMeasurement.managedObjectContext deleteObject:theMeasurement];
    [theMeasurement.managedObjectContext save:&error]; //TODO: Handle save error
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
        cell.textLabel.text = [[NSDateFormatter class] localizedStringFromDate:measurement.timestamp dateStyle:NSDateFormatterShortStyle timeStyle:NSDateFormatterShortStyle];
    } else {
        cell.textLabel.text = measurement.name;
    }
   
    switch (measurement.type.intValue)
    {
        case TypeTotalPath:
            cell.detailTextLabel.text = [TMDistanceFormatter formattedDistance:measurement.totalPath withMeasurement:measurement];
            break;
            
        case TypeHorizontal:
            cell.detailTextLabel.text = [TMDistanceFormatter formattedDistance:measurement.horzDist withMeasurement:measurement];
            break;
            
        case TypeVertical:
            cell.detailTextLabel.text = [TMDistanceFormatter formattedDistance:measurement.vertDist withMeasurement:measurement];
            break;
            
        default: //TypePointToPoint
            cell.detailTextLabel.text = [TMDistanceFormatter formattedDistance:measurement.pointToPoint withMeasurement:measurement];
            break;
    }
    
    return cell;
}


//// Override to support conditional editing of the table view.
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
        [tableView beginUpdates];
        
        [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
        
        [self deleteMeasurement:indexPath];
        [self loadTableData];
        
        [tableView endUpdates];
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
@end
