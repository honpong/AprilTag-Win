//
//  RCUserManager.h
//  RCCore
//
//  Created by Ben Hirashima on 2/5/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCHTTPClient.h"
#import "AFNetworking.h"
#import "Guid.h"
#import "KeychainItemWrapper.h"
#import "RCUser.h"
#import "NSString+RCString.h"
#import "RCHTTPClient.h"

typedef enum {
    LoginStateYes = 1, LoginStateNo = 0, LoginStateError = -1
} LoginState;

@interface RCUserManager : NSObject

- (void) fetchSessionCookie:(void (^)(NSHTTPCookie *cookie))successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock;
- (BOOL) hasValidStoredCredentials;
- (void) loginWithStoredCredentials:(void (^)())successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock;
- (void) loginWithUsername:(NSString*)username withPassword:(NSString*)password onSuccess:(void (^)())successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock;
- (void) logout;
- (LoginState) getLoginState;
- (BOOL) isUsingAnonAccount;
- (void) updateUser:(RCUser*)user onSuccess:(void (^)())successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock;;
- (void) createAnonAccount:(void (^)(NSString* username))successBlock onFailure:(void (^)(NSInteger statusCode))failureBlock;

+ (RCUserManager*) sharedInstance;

@end
