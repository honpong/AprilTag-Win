//
//  RCHTTPClient.h
//  RCCore
//
//  Created by Ben Hirashima on 2/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "AFNetworking.h"

@interface RCHTTPClient : AFHTTPClient

@property int apiVersion;

+ (void)initWithBaseUrl:(NSString*)baseUrl withAcceptHeader:(NSString*)acceptHeaderValue withApiVersion:(int)apiVersion;
+ (RCHTTPClient *) sharedInstance;

@end