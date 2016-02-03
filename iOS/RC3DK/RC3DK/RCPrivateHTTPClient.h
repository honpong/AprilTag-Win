//
//  RCHTTPClient.h
//  RC3DK
//
//  Created by Ben Hirashima on 2/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "AFNetworking.h"

@interface RCPrivateHTTPClient : RCAFHTTPClient

@property int apiVersion;

+ (void)initWithBaseUrl:(NSString*)baseUrl withAcceptHeader:(NSString*)acceptHeaderValue withApiVersion:(int)apiVersion;
+ (RCPrivateHTTPClient*) sharedInstance;

@end