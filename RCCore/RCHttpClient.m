//
//  RCHTTPClient.m
//  RCCore
//
//  Created by Ben Hirashima on 2/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCHTTPClient.h"

@implementation RCHTTPClient
@synthesize apiVersion;

static RCHTTPClient *instance;

/** Inits shared instance. If previous instance exists, it is wiped out along with it's cookies. */
+ (void)initWithBaseUrl:(NSString*)baseUrl withAcceptHeader:(NSString*)acceptHeaderValue withApiVersion:(int)apiVersion
{
    if (instance) NSLog(@"Warning: Existing instance of AFHTTPClient is being replaced. Any cookies in the previous instance are gone.");
    
    instance = [[RCHTTPClient alloc] initWithBaseURL:[NSURL URLWithString:baseUrl]];
    [instance setDefaultHeader:@"Accept" value:acceptHeaderValue];
    [instance setDefaultHeader:@"User-Agent" value:[self getUserAgentString]];
    [instance setApiVersion:apiVersion];
}

+ (NSString*)getUserAgentString
{
    NSString *appVersion = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleVersion"];
    NSString *appName = [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleIdentifier"];
    
    return [NSString stringWithFormat:@"%@/%@ (%@; iOS %@)", appName, appVersion, [RCDeviceInfo getPlatformString], [RCDeviceInfo getOSVersion]];
}

/** @returns nil if initWithBaseUrl:withAcceptHeader:withApiVersion: hasn't been called yet */
+ (RCHTTPClient *) sharedInstance
{    
    return instance;
}

@end

