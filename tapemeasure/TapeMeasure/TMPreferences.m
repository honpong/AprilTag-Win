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
    [self refreshPrefs];
}

- (void) viewDidAppear:(BOOL)animated
{
    [TMAnalytics logEvent:@"View.Preferences"];
    
    if (self.containerView) // only on iPhone
    {
        UITapGestureRecognizer *tapGesture = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(handleTapOutside:)];
        tapGesture.numberOfTapsRequired = 1;
        tapGesture.delegate = self;
        [self.view.superview addGestureRecognizer:tapGesture];
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
    
    [self.saveLocationSwitch setOn:[[NSUserDefaults.standardUserDefaults objectForKey:PREF_ADD_LOCATION] isEqual:@YES]];
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
    
    [NSUserDefaults.standardUserDefaults setObject:@(self.saveLocationSwitch.isOn) forKey:PREF_ADD_LOCATION];
}

- (IBAction)handleAllowLocationSwitch:(id)sender
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
    if(touch.view == self.containerView)
    {
        return NO;
    }
    else
    {
        return YES;
    }
}

- (void) handleTapOutside:(UIGestureRecognizer *)gestureRecognizer
{
    [self.presentingViewController dismissViewControllerAnimated:YES completion:nil];
}

@end
