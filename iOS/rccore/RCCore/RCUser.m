//
//  RCUser.m
//  RCCore
//
//  Created by Ben Hirashima on 2/20/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCUser.h"
#import "RCConstants.h"

@implementation RCUser

- (void) saveUser
{
    LOGME
    [[NSUserDefaults standardUserDefaults] setObject:self.dbid forKey:PREF_DBID];
    [[NSUserDefaults standardUserDefaults] setObject:self.username forKey:PREF_USERNAME];
    [[NSUserDefaults standardUserDefaults] setObject:self.firstName forKey:PREF_FIRST_NAME];
    [[NSUserDefaults standardUserDefaults] setObject:self.lastName forKey:PREF_LAST_NAME];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    if (self.password) {
        KeychainItemWrapper *kcItem = [[KeychainItemWrapper alloc] initWithIdentifier:KEYCHAIN_ITEM_IDENTIFIER accessGroup:nil];
        [kcItem setObject:(__bridge id)(kSecAttrAccessibleWhenUnlocked) forKey:(__bridge id)(kSecAttrAccessible)];
        [kcItem setObject:self.password forKey:(__bridge NSString*)kSecValueData];
    }
}

+ (RCUser*) getStoredUser
{
    RCUser *user = [[RCUser alloc] init];
    
    user.dbid = [[NSUserDefaults standardUserDefaults] objectForKey:PREF_DBID];
    user.username = [[NSUserDefaults standardUserDefaults] objectForKey:PREF_USERNAME];
    user.firstName = [[NSUserDefaults standardUserDefaults] objectForKey:PREF_FIRST_NAME];
    user.lastName = [[NSUserDefaults standardUserDefaults] objectForKey:PREF_LAST_NAME];
    
    KeychainItemWrapper *kcItem = [[KeychainItemWrapper alloc] initWithIdentifier:KEYCHAIN_ITEM_IDENTIFIER accessGroup:nil];
    if (kcItem) user.password = [kcItem objectForKey:(__bridge NSString*)kSecValueData];
    
    return user;
}

+ (void) deleteStoredUser
{
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:PREF_DBID];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:PREF_USERNAME];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:PREF_FIRST_NAME];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:PREF_LAST_NAME];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
    KeychainItemWrapper *kcItem = [[KeychainItemWrapper alloc] initWithIdentifier:KEYCHAIN_ITEM_IDENTIFIER accessGroup:nil];
    [kcItem resetKeychainItem];
}

+ (BOOL) isValidEmail: (NSString *)email
{
    if (email == nil || email.length < 6) return NO;
    
    BOOL stricterFilter = NO; // Discussion http://blog.logichigh.com/2010/09/02/validating-an-e-mail-address/
    NSString *stricterFilterString = @"[A-Z0-9a-z._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,4}";
    NSString *laxString = @".+@.+\\.[A-Za-z]{2}[A-Za-z]*";
    NSString *emailRegex = stricterFilter ? stricterFilterString : laxString;
    NSPredicate *emailTest = [NSPredicate predicateWithFormat:@"SELF MATCHES %@", emailRegex];
    return [emailTest evaluateWithObject:email];
}

@end
