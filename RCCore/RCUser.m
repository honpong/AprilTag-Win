//
//  RCUser.m
//  RCCore
//
//  Created by Ben Hirashima on 2/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCUser.h"

@implementation RCUser

- (void) saveUser
{
    [[NSUserDefaults standardUserDefaults] setObject:self.dbid forKey:PREF_DBID];
    [[NSUserDefaults standardUserDefaults] setObject:self.username forKey:PREF_USERNAME];
    [[NSUserDefaults standardUserDefaults] setObject:self.password forKey:PREF_PASSWORD]; //TODO:store securely in keychain
    [[NSUserDefaults standardUserDefaults] setObject:self.firstName forKey:PREF_FIRST_NAME];
    [[NSUserDefaults standardUserDefaults] setObject:self.lastName forKey:PREF_LAST_NAME];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    NSLog(@"User saved");
}

+ (RCUser*)getStoredUser
{
    RCUser *user = [[RCUser alloc] init];
    
    user.dbid = [[NSUserDefaults standardUserDefaults] objectForKey:PREF_DBID];
    user.username = [[NSUserDefaults standardUserDefaults] objectForKey:PREF_USERNAME];
    user.password = [[NSUserDefaults standardUserDefaults] objectForKey:PREF_PASSWORD];
    user.firstName = [[NSUserDefaults standardUserDefaults] objectForKey:PREF_FIRST_NAME];
    user.lastName = [[NSUserDefaults standardUserDefaults] objectForKey:PREF_LAST_NAME];
    
    return user;
}

+ (void)deleteStoredUser
{
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:PREF_DBID];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:PREF_USERNAME];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:PREF_PASSWORD];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:PREF_FIRST_NAME];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:PREF_LAST_NAME];
    [[NSUserDefaults standardUserDefaults] synchronize];
}

@end
