//
//  RCHttpClientFactory.h
//  RCCore
//
//  Created by Ben Hirashima on 2/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "AFNetworking.h"

@interface RCHttpClientFactory : NSObject

+ (void)initWithBaseUrl:(NSString*)baseUrl andAcceptHeader:(NSString*)acceptHeaderValue;
+ (AFHTTPClient*)getInstance;
+ (void)setInstance:(AFHTTPClient*)mockObject;

@end