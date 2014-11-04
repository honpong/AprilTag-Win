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

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    
    [self.allowLocationSwitch addTarget:self action:@selector(setSaveLocationSwitchEnabledStatus) forControlEvents:UIControlEventValueChanged];
    [self refreshPrefs];
}

- (void) viewDidAppear:(BOOL)animated
{
    [TMAnalytics logEvent:@"View.Preferences"];
    
    if (self.modalPresentationStyle == UIModalPresentationCustom)
    {
        UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapOutside:)];
        tapGesture.numberOfTapsRequired = 1;
        tapGesture.delegate = self;
        [self.view addGestureRecognizer:tapGesture];
    }
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
    
    [self.saveLocationSwitch setOn:[NSUserDefaults.standardUserDefaults boolForKey:PREF_ADD_LOCATION]];
    
    BOOL shouldAllowLocationSwitchBeOn = [NSUserDefaults.standardUserDefaults boolForKey:PREF_USE_LOCATION] && [LOCATION_MANAGER isLocationExplicitlyAllowed];
    [self.allowLocationSwitch setOn:shouldAllowLocationSwitchBeOn];
    
    [self setSaveLocationSwitchEnabledStatus];
}

- (IBAction)handleUnitsControl:(id)sender
{
    if (self.unitsControl.selectedSegmentIndex == 0)
    {
        [TMAnalytics logEvent:@"Preferences" withParameters:@{ @"Units" : @"Metric" }];
        [NSUserDefaults.standardUserDefaults setObject:@(UnitsMetric) forKey:PREF_UNITS];
    }
    else if (self.unitsControl.selectedSegmentIndex == 1)
    {
        [TMAnalytics logEvent:@"Preferences" withParameters:@{ @"Units" : @"Imperial" }];
        [NSUserDefaults.standardUserDefaults setObject:@(UnitsImperial) forKey:PREF_UNITS];
    }
    
    [NSUserDefaults.standardUserDefaults synchronize];
}

- (IBAction)handleSaveLocationSwitch:(id)sender
{
    if (self.saveLocationSwitch.isOn)
        [TMAnalytics logEvent:@"Preferences" withParameters:@{ @"Location" : @"On" }];
    else
        [TMAnalytics logEvent:@"Preferences" withParameters:@{ @"Location" : @"Off" }];
    
    [NSUserDefaults.standardUserDefaults setBool:self.saveLocationSwitch.isOn forKey:PREF_ADD_LOCATION];
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
                [self setSaveLocationSwitchEnabledStatus]; // shouldn't be necessary, but is for some reason
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
                        [self setSaveLocationSwitchEnabledStatus]; // shouldn't be necessary, but is for some reason
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
            [self setSaveLocationSwitchEnabledStatus]; // shouldn't be necessary, but is for some reason
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
            [self setSaveLocationSwitchEnabledStatus]; // shouldn't be necessary, but is for some reason
        }
        else
        {
            [self.allowLocationSwitch setOn:NO animated:YES];
            [self setSaveLocationSwitchEnabledStatus]; // shouldn't be necessary, but is for some reason
        }
    }];
}

- (void) setSaveLocationSwitchEnabledStatus
{
    if (self.allowLocationSwitch.isOn)
    {
        self.saveLocationSwitch.enabled = YES;
        self.saveLocationLabel.alpha = 1.;
    }
    else
    {
        self.saveLocationSwitch.enabled = NO;
        self.saveLocationLabel.alpha = .5;
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

@end
