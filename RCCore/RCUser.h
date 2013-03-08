//
//  RCUser.h
//  RCCore
//
//  Created by Ben Hirashima on 2/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "KeychainItemWrapper.h"

@interface RCUser : NSObject

@property NSNumber *dbid;
@property NSString *username;
@property NSString *password;
@property NSString *firstName;
@property NSString *lastName;

- (void) saveUser;
+ (RCUser*) getStoredUser;
+ (void) deleteStoredUser;
+ (BOOL) isValidEmail: (NSString *)email;

@end
