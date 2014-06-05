//
//  TMPreferences.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 6/5/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "TMPreferences.h"

@interface TMPreferences ()

@end

@implementation TMPreferences

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
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    [self refreshPrefs];
}

- (void) handleResume
{
    [self refreshPrefs];
}

- (void) refreshPrefs
{
    [NSUserDefaults.standardUserDefaults synchronize];
    
    if ([[NSUserDefaults.standardUserDefaults objectForKey:PREF_UNITS] isEqual: @(UnitsImperial)])
    {
        self.unitsControl.selectedSegmentIndex = 1;
    }
    else
    {
        self.unitsControl.selectedSegmentIndex = 0;
    }
    
    [self.locationSwitch setOn:[[NSUserDefaults.standardUserDefaults objectForKey:PREF_ADD_LOCATION] isEqual:@YES]];
}

- (IBAction)handleUnitsControl:(id)sender
{
    if (self.unitsControl.selectedSegmentIndex == 0)
    {
        [NSUserDefaults.standardUserDefaults setObject:@(UnitsMetric) forKey:PREF_UNITS];
    }
    else if (self.unitsControl.selectedSegmentIndex == 1)
    {
        [NSUserDefaults.standardUserDefaults setObject:@(UnitsImperial) forKey:PREF_UNITS];
    }
    
    [NSUserDefaults.standardUserDefaults synchronize];
}

- (IBAction)handleLocationSwitch:(id)sender
{
    [NSUserDefaults.standardUserDefaults setObject:@(self.locationSwitch.isOn) forKey:PREF_ADD_LOCATION];
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
    return 2;
}

/*
- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:<#@"reuseIdentifier"#> forIndexPath:indexPath];
    
    // Configure the cell...
    
    return cell;
}
*/

/*
// Override to support conditional editing of the table view.
- (BOOL)tableView:(UITableView *)tableView canEditRowAtIndexPath:(NSIndexPath *)indexPath
{
    // Return NO if you do not want the specified item to be editable.
    return YES;
}
*/

/*
// Override to support editing the table view.
- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath
{
    if (editingStyle == UITableViewCellEditingStyleDelete) {
        // Delete the row from the data source
        [tableView deleteRowsAtIndexPaths:@[indexPath] withRowAnimation:UITableViewRowAnimationFade];
    } else if (editingStyle == UITableViewCellEditingStyleInsert) {
        // Create a new instance of the appropriate class, insert it into the array, and add a new row to the table view
    }   
}
*/

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

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

@end
