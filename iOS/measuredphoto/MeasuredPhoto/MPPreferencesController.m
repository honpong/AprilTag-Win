//
//  MPPreferencesController.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 9/23/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPPreferencesController.h"
#import <RCCore/RCCore.h>
#import "MPAnalytics.h"
#import "MPConstants.h"

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
    [MPAnalytics logEvent:@"View.Preferences"];
    
    if (self.modalPresentationStyle == UIModalPresentationCustom)
    {
        UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapOutside:)];
        tapGesture.numberOfTapsRequired = 1;
        tapGesture.delegate = self;
        [self.view addGestureRecognizer:tapGesture];
    }
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch
{
    return touch.view == self.containerView ? NO : YES;
}

- (void) handleTapOutside:(UIGestureRecognizer *)gestureRecognizer
{
    [self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
}

- (void) handleResume
{
    [self refreshPrefs];
}

- (void) refreshPrefs
{
    if ([[NSUserDefaults.standardUserDefaults objectForKey:PREF_UNITS] isEqual: @(UnitsImperial)])
    {
        self.unitsControl.selectedSegmentIndex = 1;
    }
    else
    {
        self.unitsControl.selectedSegmentIndex = 0;
    }
    
    BOOL shouldAllowLocationSwitchBeOn = [NSUserDefaults.standardUserDefaults boolForKey:PREF_USE_LOCATION] && [LOCATION_MANAGER isLocationExplicitlyAllowed];
    [self.allowLocationSwitch setOn:shouldAllowLocationSwitchBeOn];
}

- (IBAction)handleUnitsControl:(id)sender
{
    if (self.unitsControl.selectedSegmentIndex == 0)
    {
        [MPAnalytics logEvent:@"Preferences" withParameters:@{ @"Units" : @"Metric" }];
        [NSUserDefaults.standardUserDefaults setObject:@(UnitsMetric) forKey:PREF_UNITS];
    }
    else if (self.unitsControl.selectedSegmentIndex == 1)
    {
        [MPAnalytics logEvent:@"Preferences" withParameters:@{ @"Units" : @"Imperial" }];
        [NSUserDefaults.standardUserDefaults setObject:@(UnitsImperial) forKey:PREF_UNITS];
    }
    
    [NSUserDefaults.standardUserDefaults synchronize];
}

- (IBAction)handleAllowLocationSwitch:(id)sender
{
    if (self.allowLocationSwitch.isOn)
    {
        if (![CLLocationManager locationServicesEnabled])
        {
            if ([NSUserDefaults.standardUserDefaults boolForKey:PREF_PROMPTED_LOCATION_SERVICES])
            {
                UIAlertView* alertView = [[UIAlertView alloc] initWithTitle:@"Whoops"
                                                                    message:@"Location services are currently disabled for the entire device. Please go to Settings > Privacy > Location Services to enable location."
                                                                   delegate:self
                                                          cancelButtonTitle:nil
                                                          otherButtonTitles:@"Got it", nil];
                [alertView show];
                
                [self.allowLocationSwitch setOn:NO animated:YES];
            }
            else
            {
                [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_PROMPTED_LOCATION_SERVICES];
                
                [LOCATION_MANAGER requestLocationAccessWithCompletion:^(BOOL authorized) {
                    [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_SHOW_LOCATION_NAG];
                    if(authorized)
                    {
                        [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_USE_LOCATION];
                        [self.allowLocationSwitch setOn:YES animated:NO];
                    }
                }];
            }
        }
        else if ([LOCATION_MANAGER isLocationDisallowed])
        {
            UIAlertView* alertView = [[UIAlertView alloc] initWithTitle:@"Denied"
                                                                message:@"This app has been denied location permission, possibly by you. Please go to [Settings > Privacy > Location Services] to grant location permission to this app."
                                                               delegate:self
                                                      cancelButtonTitle:nil
                                                      otherButtonTitles:@"Got it", nil];
            [alertView show];
            
            [self.allowLocationSwitch setOn:NO animated:YES];
        }
        else if (![LOCATION_MANAGER isLocationExplicitlyAllowed])
        {
            [self requestLocationPermission];
        }
        else
        {
            [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_USE_LOCATION];
        }
    }
    else
    {
        [NSUserDefaults.standardUserDefaults setBool:self.allowLocationSwitch.isOn forKey:PREF_USE_LOCATION];
    }
}

- (void) requestLocationPermission
{
    [LOCATION_MANAGER requestLocationAccessWithCompletion:^(BOOL authorized) {
        [NSUserDefaults.standardUserDefaults setBool:NO forKey:PREF_SHOW_LOCATION_NAG];
        if(authorized)
        {
            [NSUserDefaults.standardUserDefaults setBool:YES forKey:PREF_USE_LOCATION];
            [LOCATION_MANAGER startLocationUpdates];
            [self.allowLocationSwitch setOn:YES animated:YES];
        }
        else
        {
            [self.allowLocationSwitch setOn:NO animated:YES];
        }
    }];
}

@end
