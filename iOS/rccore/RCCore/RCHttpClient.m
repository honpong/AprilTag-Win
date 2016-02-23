//
//  RCHTTPClient.m
//  RCCore
//
//  Created by Ben Hirashima on 2/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCHTTPClient.h"
#import "RCConstants.h"
#import "RC3DK.h"

@implementation RCHTTPClient
@synthesize apiVersion;

static RCHTTPClient *instance;

/** Inits shared instance. If previous instance exists, it is wiped out along with it's cookies. */
+ (void)initWithBaseUrl:(NSString*)baseUrl withAcceptHeader:(NSString*)acceptHeaderValue withApiVersion:(int)apiVersion
{
    if (instance) DLog(@"Warning: Existing instance of AFHTTPClient is being replaced. Any cookies in the previous instance are gone.");
    
    NSString* userAgent = [self getUserAgentString];
    
    instance = [[RCHTTPClient alloc] initWithBaseURL:[NSURL URLWithString:baseUrl]];
    [instance setDefaultHeader:@"Accept" value:acceptHeaderValue];
    [instance setDefaultHeader:@"User-Agent" value:userAgent];
    [instance setApiVersion:apiVersion];
    
    DLog(@"\nBase URL: %@\nAccept-Header: %@\nUser-Agent: %@\nAPI-Version: %i\n", instance.baseURL, acceptHeaderValue, userAgent, apiVersion);
}

+ (NSString*)getUserAgentString
{
    NSString *appVersion = [[NSBundle mainBundle] infoDictionary][@"CFBundleVersion"];
    NSString *appName = [[NSBundle mainBundle] infoDictionary][@"CFBundleIdentifier"];
    
    return [NSString stringWithFormat:@"%@/%@ (%@; iOS %@)", appName, appVersion, [RCDeviceInfo getPlatformString], [RCDeviceInfo getOSVersion]];
}

/** @returns nil if initWithBaseUrl:withAcceptHeader:withApiVersion: hasn't been called yet */
+ (RCHTTPClient *) sharedInstance
{    
    return instance;
}

@end

