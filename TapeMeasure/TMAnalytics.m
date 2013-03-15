//
//  TMAnalytics.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/15/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMAnalytics.h"

@implementation TMAnalytics

+ (void) logEvent: (NSString*)eventName
{
    NSLog(@"Analytics: %@", eventName);
    [Flurry logEvent:eventName];
}

+ (void) logEvent: (NSString*)eventName withParameters: (NSDictionary*)params
{
    NSLog(@"Analytics: %@", eventName);
    [Flurry logEvent:eventName withParameters:params];
}

+ (void) logError: (NSString*) eventName message: (NSString*)message error: (NSError*)error
{
    NSLog(@"Analytics: %@\nError: %@", eventName, error.debugDescription);
    [Flurry logError:eventName message:message error:error];
}

+ (void) logError: (NSString*) eventName message: (NSString*)message exception: (NSException*)exception
{
    NSLog(@"Analytics: %@\nError: %@", eventName, exception.debugDescription);
    [Flurry logError:eventName message:message exception:exception];
}

@end
