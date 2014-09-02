//
//  TMLocationIntro.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 8/28/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "TMLocationIntro.h"
@import CoreLocation;

@interface TMLocationIntro ()

@end

@implementation TMLocationIntro

- (id) initWithCoder:(NSCoder *)aDecoder
{
    if (self = [super initWithCoder:aDecoder])
    {
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleResume)
                                                     name:UIApplicationDidBecomeActiveNotification
                                                   object:nil];
    }
    return self;
}

- (BOOL) prefersStatusBarHidden { return YES; }

- (void) viewDidLoad
{
    [super viewDidLoad];
}

- (void) viewDidAppear:(BOOL)animated
{
    [self setIntroText];
}

- (void) handleResume
{
    [self setIntroText];
}

- (IBAction) handleNextButton:(id)sender
{
    if ([self.delegate respondsToSelector:@selector(nextButtonTapped)])
    {
        [self.delegate nextButtonTapped];
    }
}

- (void) setIntroText
{
    if ([CLLocationManager locationServicesEnabled])
    {
        self.introLabel.text = @"Welcome. When you tap Next, we will ask for permission to use your  current location. This is optional, but it makes your measurements more accurate by adjusting for differences in gravity across the earth.";
    }
    else
    {
        self.introLabel.text = @"Welcome. I see that you have location services disabled. If you allow this app to use your location, it can make your measurements more accurate. This is completely optional. You can enable location in the Settings app, under 'Privacy'.";
    }
    
    [self.introLabel sizeToFit];
}

@end
