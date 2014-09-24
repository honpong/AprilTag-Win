//
//  MPPreferencesController.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPPreferencesController.h"
#import <RCCore/RCCore.h>

@interface MPPreferencesController ()

@end

@implementation MPPreferencesController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    [self refreshPrefs];
}

- (void) viewDidAppear:(BOOL)animated
{
//    [TMAnalytics logEvent:@"View.Preferences"];
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
}

- (IBAction)handleUnitsControl:(id)sender
{
    if (self.unitsControl.selectedSegmentIndex == 0)
    {
//        [TMAnalytics logEvent:@"Preferences" withParameters:@{ @"Units" : @"Metric" }];
        [NSUserDefaults.standardUserDefaults setObject:@(UnitsMetric) forKey:PREF_UNITS];
    }
    else if (self.unitsControl.selectedSegmentIndex == 1)
    {
//        [TMAnalytics logEvent:@"Preferences" withParameters:@{ @"Units" : @"Imperial" }];
        [NSUserDefaults.standardUserDefaults setObject:@(UnitsImperial) forKey:PREF_UNITS];
    }
    
    [NSUserDefaults.standardUserDefaults synchronize];
}

- (IBAction)handleBackButton:(id)sender
{
    [self.presentingViewController dismissViewControllerAnimated:YES completion:NO];
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
    return 1;
}

@end
