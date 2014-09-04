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
{
    NSString* originalTextFromStoryboard;
}

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

- (NSUInteger) supportedInterfaceOrientations { return UIInterfaceOrientationMaskPortrait; }

- (void) viewDidLoad
{
    [super viewDidLoad];
    
    originalTextFromStoryboard = self.introLabel.text;
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

- (IBAction)handleLaterButton:(id)sender
{
    
}

- (void) setIntroText
{
    if ([CLLocationManager locationServicesEnabled])
    {
        self.introLabel.text = originalTextFromStoryboard;
    }
    else
    {
        self.introLabel.text = @"Welcome. I see that you have location services disabled. If you allow this app to use your location, it can make your measurements more accurate. This is completely optional. You can enable location in the Settings app, under 'Privacy'.";
    }
    
    [self.introLabel sizeToFit];
}

@end
