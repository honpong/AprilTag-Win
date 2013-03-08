//
//  RCUserManagerFactory.h
//  RCCore
//
//  Created by Ben Hirashima on 2/5/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCHttpClientFactory.h"
#import "AFNetworking.h"
#import "Guid.h"
#import "KeychainItemWrapper.h"
#import "RCUser.h"
#import "NSString+RCString.h"
#import "RCHTTPClient.h"

@protocol RCUserManager <NSObject>

- (void) fetchSessionCookie:(void (^)(NSHTTPCookie *cookie))successBlock onFailure:(void (^)(int statusCode))failureBlock;
- (BOOL) hasValidStoredCredentials;
- (void) loginWithStoredCredentials:(void (^)())successBlock onFailure:(void (^)(int statusCode))failureBlock;
- (void) loginWithUsername:(NSString*)username withPassword:(NSString*)password onSuccess:(void (^)())successBlock onFailure:(void (^)(int statusCode))failureBlock;
- (void) logout;
- (BOOL) isLoggedIn;
- (BOOL) isUsingAnonAccount;
- (void) updateUser:(RCUser*)user onSuccess:(void (^)())successBlock onFailure:(void (^)(int statusCode))failureBlock;;
- (void) createAnonAccount:(void (^)(NSString* username))successBlock onFailure:(void (^)(int statusCode))failureBlock;

@end

@interface RCUserManagerFactory : NSObject

+ (id<RCUserManager>)getInstance;
+ (void)setInstance:(id<RCUserManager>)mockObject;

@end
