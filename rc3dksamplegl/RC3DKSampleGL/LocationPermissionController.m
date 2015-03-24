//
//  LocationPermissionController.m
//  RC3DKSampleGL
//
//  Created by Ben Hirashima on 9/15/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "LocationPermissionController.h"
#import <QuickstartKit/QuickstartKit.h>

@implementation LocationPermissionController
{
    NSString* originalTextFromStoryboard;
    NSString* originalNextButtonText;
    NSString* originalNeverButtonText;
}

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        
    }
    return self;
}

- (void) viewDidLoad
{
    [super viewDidLoad];
    
    originalTextFromStoryboard = self.introLabel.text;
    originalNextButtonText = self.allowButton.titleLabel.text;
    originalNeverButtonText = self.neverButton.titleLabel.text;
    
    [self setIntroText];
}

- (void) viewDidAppear:(BOOL)animated
{
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handlePause)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    
    [self setIntroText];
}

- (void) viewWillDisappear:(BOOL)animated
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) handleResume
{
    [self setIntroText];
}

- (void) handlePause
{
    
}

- (IBAction) handleAllowButton:(id)sender
{
    [[RCLocationManager sharedInstance] requestLocationAccessWithCompletion:^(BOOL granted)
     {
        if ([self.delegate respondsToSelector:@selector(locationPermissionResult:)])
        {
            [self.delegate locationPermissionResult:granted];
        }
     }];
}

- (IBAction)handleNeverButton:(id)sender
{
    if ([self.delegate respondsToSelector:@selector(locationPermissionResult:)])
    {
        [self.delegate locationPermissionResult:NO];
    }
}

- (void) setIntroText
{
    if ([CLLocationManager locationServicesEnabled])
    {
        [self.allowButton setTitle:originalNextButtonText forState:UIControlStateNormal];
        [self.neverButton setTitle:originalNeverButtonText forState:UIControlStateNormal];
        self.introLabel.text = originalTextFromStoryboard;
    }
    else
    {
        [self.allowButton setTitle:@"Turn on location services" forState:UIControlStateNormal];
        [self.neverButton setTitle:@"Skip" forState:UIControlStateNormal];
        self.introLabel.text = @"I see that you have location services disabled. If you allow this app to use your location, it can make your measurements more accurate by adjusting for differences in gravity across the earth. This is optional, but recommended";
    }
    
    [self.introLabel sizeToFit];
}

@end
