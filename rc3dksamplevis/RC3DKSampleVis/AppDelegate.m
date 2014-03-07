//
//  AppDelegate.m
//  RC3DKSampleVis
//
//  Created by Brian on 8/26/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import "AppDelegate.h"

#import "GCDAsyncSocket.h"

#import "ConnectionManager.h"

#import <CoreFoundation/CoreFoundation.h>

@implementation AppDelegate

#define TAG_FIXED_LENGTH_HEADER 0
#define TAG_MESSAGE_BODY 1
#define headerLength 8

@synthesize glview, topDownViewMenuItem, sideViewMenuItem, animateViewMenuItem, allFeaturesMenuItem, filterFeaturesMenuItem;

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    [[ConnectionManager sharedInstance] startListening];
}

- (IBAction)handleViewChange:(id)sender
{
    [topDownViewMenuItem setState:NSOffState];
    [sideViewMenuItem setState:NSOffState];
    [animateViewMenuItem setState:NSOffState];

    if(sender == topDownViewMenuItem)
        [glview setViewpoint:RCViewpointTopDown];
    else if (sender == sideViewMenuItem)
        [glview setViewpoint:RCViewpointSide];
    else //if (sender == animateViewMenuItem)
        [glview setViewpoint:RCViewpointAnimating];
    [sender setState:NSOnState];
}

- (IBAction)handleFeatureChange:(id)sender
{
    [allFeaturesMenuItem setState:NSOffState];
    [filterFeaturesMenuItem setState:NSOffState];

    if(sender == allFeaturesMenuItem)
        [glview setFeatureFilter:RCFeatureFilterShowAll];
    else
        [glview setFeatureFilter:RCFeatureFilterShowGood];
    [sender setState:NSOnState];
}

@end
