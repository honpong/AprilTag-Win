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

@protocol RCUserManager <NSObject>

- (void) fetchSessionCookie:(void (^)())successBlock onFailure:(void (^)(int))failureBlock;
- (BOOL) hasValidStoredCredentials;
- (RCUser*) getStoredUser;
- (void) loginWithStoredCredentials:(void (^)())successBlock onFailure:(void (^)(int))failureBlock;
- (void) loginWithUsername:(NSString*)username withPassword:(NSString*)password onSuccess:(void (^)())successBlock onFailure:(void (^)(int))failureBlock;
- (void) logout;
- (BOOL) isLoggedIn;
- (BOOL) isUsingAnonAccount;
- (void) createAccount:(RCUser*)user onSuccess:(void (^)(int userId))successBlock onFailure:(void (^)(int statusCode))failureBlock;;
- (void) updateUser:(RCUser*)user onSuccess:(void (^)())successBlock onFailure:(void (^)(int))failureBlock;;
- (void) createAnonAccount:(void (^)(NSString* username))successBlock onFailure:(void (^)(int))failureBlock;

@end

@interface RCUserManagerFactory : NSObject

+ (id<RCUserManager>)getInstance;
+ (void)setInstance:(id<RCUserManager>)mockObject;

@end
