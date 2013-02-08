//
//  RCHttpClientFactory.m
//  RCCore
//
//  Created by Ben Hirashima on 2/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCHttpClientFactory.h"

@implementation RCHttpClientFactory

static AFHTTPClient *instance;

/** Inits instance. If previous instance exists, it is wiped out along with it's cookies. */
+ (void)initWithBaseUrl:(NSString*)baseUrl andAcceptHeader:(NSString*)acceptHeaderValue
{
    if (instance) NSLog(@"Warning: Existing instance of AFHTTPClient is being replaced. Any cookies in the previous instance are gone.");
    
    instance = [[AFHTTPClient alloc] initWithBaseURL:[NSURL URLWithString:baseUrl]];
    [instance setDefaultHeader:@"Accept" value:acceptHeaderValue];
}

/** @returns nil if initWithBaseUrl:andAcceptHeader: hasn't been called yet */
+ (AFHTTPClient*)getInstance
{    
    return instance;
}

+ (void)setInstance:(AFHTTPClient*)mockObject
{
    instance = mockObject;
}

@end

