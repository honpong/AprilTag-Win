//
//  RCHTTPClient.m
//  RC3DK
//
//  Created by Ben Hirashima on 2/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCPrivateHTTPClient.h"
#import "RCDeviceInfo.h"

@implementation RCPrivateHTTPClient
@synthesize apiVersion;

static RCPrivateHTTPClient*instance;

+ (void)initWithBaseUrl:(NSString*)baseUrl withAcceptHeader:(NSString*)acceptHeaderValue withApiVersion:(int)apiVersion
{
    if (instance) DLog(@"Warning: Existing instance of RCAFHTTPClient is being replaced. Any cookies in the previous instance are gone.");
    
    NSString* userAgent = [self getUserAgentString];
    
    instance = [[RCPrivateHTTPClient alloc] initWithBaseURL:[NSURL URLWithString:baseUrl]];
    [instance setDefaultHeader:@"Accept" value:acceptHeaderValue];
    [instance setDefaultHeader:@"User-Agent" value:userAgent];
    [instance setApiVersion:apiVersion];
    
//    DLog(@"\nBase URL: %@\nAccept-Header: %@\nUser-Agent: %@\nAPI-Version: %i\n", baseUrl, acceptHeaderValue, userAgent, apiVersion);
}

+ (NSString*)getUserAgentString
{
    NSString *appName = @"3DK";
    return [NSString stringWithFormat:@"%@/%@ (%@; iOS %@)", appName, RC3DK_VERSION, [RCDeviceInfo getPlatformString], [RCDeviceInfo getOSVersion]];
}

+ (RCPrivateHTTPClient*) sharedInstance
{    
    return instance;
}

@end

