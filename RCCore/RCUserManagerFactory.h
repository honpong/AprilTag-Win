//
//  RCUserManagerFactory.h
//  RCCore
//
//  Created by Ben Hirashima on 2/5/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCHttpClientFactory.h"
#import "AFNetworking.h"

@protocol RCUserManager <NSObject>

- (void)fetchSessionCookie:(void (^)())successBlock onFailure:(void (^)(int))failureBlock;
- (void)loginWithUsername:(NSString*)username withPassword:(NSString*)password onSuccess:(void (^)())successBlock onFailure:(void (^)(int))failureBlock;
- (void)logout;
- (BOOL)isLoggedIn;
- (void)createAccount:(NSString*)username withPassword:(NSString*)password onSuccess:(void (^)())successBlock onFailure:(void (^)(int))failureBlock;;
- (void)changeUsername:(NSString*)username andPassword:(NSString*)password onSuccess:(void (^)())successBlock onFailure:(void (^)(int))failureBlock;;

@end

@interface RCUserManagerFactory : NSObject

+ (id<RCUserManager>)getInstance;
+ (void)setInstance:(id<RCUserManager>)mockObject;

@end
