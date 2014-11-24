//
//  MPAnalytics.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/15/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPAnalytics.h"
#import "Flurry.h"

@implementation MPAnalytics

+ (void) logEvent: (NSString*)eventName
{
    DLog(@"%@", eventName);
    [Flurry logEvent:eventName];
}

+ (void) logEvent: (NSString*)eventName withParameters: (NSDictionary*)params
{
    DLog(@"%@ %@", eventName, params);
    [Flurry logEvent:eventName withParameters:params];
}

+ (void) logError: (NSString*) eventName message: (NSString*)message error: (NSError*)error
{
    DLog(@"%@\nError: %@", eventName, error.debugDescription);
    [Flurry logError:eventName message:message error:error];
}

+ (void) logError: (NSString*) eventName message: (NSString*)message exception: (NSException*)exception
{
    DLog(@"%@\nError: %@", eventName, exception.debugDescription);
    [Flurry logError:eventName message:message exception:exception];
}

+ (void) startTimedEvent: (NSString*)eventName withParameters: (NSDictionary*)params
{
    DLog(@"%@ %@", eventName, params);
    [Flurry logEvent:eventName withParameters:params timed:YES];
}

+ (void) endTimedEvent: (NSString*)eventName
{
    [Flurry endTimedEvent:eventName withParameters:nil];
}

@end
