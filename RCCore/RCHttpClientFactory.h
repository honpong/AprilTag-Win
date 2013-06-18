//
//  RCHttpClientFactory.h
//  RCCore
//
//  Created by Ben Hirashima on 2/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "AFNetworking.h"
#import "RCDeviceInfo.h"
#import "RCHTTPClient.h"

@interface RCHttpClientFactory : NSObject

+ (void)initWithBaseUrl:(NSString*)baseUrl withAcceptHeader:(NSString*)acceptHeaderValue withApiVersion:(int)apiVersion;
+ (RCHTTPClient*)getInstance;

#ifdef DEBUG
+ (void)setInstance:(RCHTTPClient*)mockObject;
#endif

@end