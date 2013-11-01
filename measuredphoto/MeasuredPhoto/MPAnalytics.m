//
//  TMAnalytics.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/15/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPAnalytics.h"
#import "GAIDictionaryBuilder.h"

@implementation MPAnalytics

+ (id<GAITracker>) getTracker
{
    return [[GAI sharedInstance] trackerWithTrackingId:@"UA-43198622-1"];
}

+ (void) logEventWithCategory:(NSString*)category withAction:(NSString*)action withLabel:(NSString*)label withValue:(NSNumber*)value
{
    DLog(@"%@, %@, %@, %@", category, action, label, value);
	[[self getTracker] send:[[GAIDictionaryBuilder createEventWithCategory:category     // Event category (required)
                                                                    action:action       // Event action (required)
                                                                     label:label        // Event label
                                                                     value:value] build]];
}

+ (void) logError:(NSString*)errorType withMessage:(NSString*)errorMessage
{
    DLog(@"%@\n%@", errorType, errorMessage);
    [self logEventWithCategory:@"Error" withAction:errorType withLabel:errorMessage withValue:nil];
}

+ (void) logError:(NSString*)errorType withError:(NSError*)error
{
    DLog(@"%@\n%@", errorType, error.debugDescription);
    [self logEventWithCategory:@"Error" withAction:errorType withLabel:error.debugDescription withValue:nil];
}

+ (void) logError:(NSString*)errorType withException: (NSException*)exception
{
    DLog(@"%@\n%@", errorType, exception.debugDescription);
    [self logEventWithCategory:@"Error" withAction:errorType withLabel:exception.debugDescription withValue:nil];
}

/** Send analytics data to server right now */
+ (void) dispatch
{
    [[GAI sharedInstance] dispatch];
}

@end
