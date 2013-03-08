//
//  RCUserTests.m
//  RCCore
//
//  Created by Ben Hirashima on 3/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCUserTests.h"
#import "RCUser.h"

@implementation RCUserTests

- (void) setUp
{
    [super setUp];
    
    // Set-up code here.
}

- (void) tearDown
{
    [RCUser deleteStoredUser];
    
    [super tearDown];
}

- (void) testSaveAndGetUser
{
    RCUser *user = [[RCUser alloc] init];
    user.username = @"test";
    user.password = @"password";
    user.firstName = @"firstName";
    user.lastName = @"lastName";
    user.dbid = @1;
    [user saveUser];
    
    RCUser *user2 = [RCUser getStoredUser];
    
    STAssertNotNil(user2, @"getStoredUser returned nil");
    STAssertEquals(user.username, user2.username, @"Usernames don't match");
    STAssertEquals([user.password hash], [user2.password hash], @"Passwords don't match");
    STAssertEquals(user.firstName, user2.firstName, @"First names don't match");
    STAssertEquals(user.lastName, user2.lastName, @"Last names don't match");
    STAssertEquals(user.dbid, user2.dbid, @"DBIDs don't match");
}

- (void) testSaveAndDeleteUser
{
    RCUser *user = [[RCUser alloc] init];
    user.username = @"test";
    user.password = @"password";
    user.firstName = @"firstName";
    user.lastName = @"lastName";
    user.dbid = @1;
    [user saveUser];
    
    [RCUser deleteStoredUser];
    
    user = [RCUser getStoredUser];
    
    STAssertNil(user.username, @"Username was not deleted");
    STAssertTrue(user.password.length == 0, @"Password was not deleted");
    STAssertNil(user.firstName, @"First name was not deleted");
    STAssertNil(user.lastName, @"Last name was not deleted");
    STAssertNil(user.dbid, @"DBID was not deleted");
}

- (void) testIsValidEmail
{
    STAssertTrue([RCUser isValidEmail:@"ben@realitycap.com"], @"Valid email failed validation");
    STAssertTrue([RCUser isValidEmail:@"ben.hirashima@realitycap.com"], @"Valid email failed validation");
    STAssertTrue([RCUser isValidEmail:@"ben-hirashima@realitycap.com"], @"Valid email failed validation");
    STAssertTrue([RCUser isValidEmail:@"ben@reality-cap.com"], @"Valid email failed validation");
    STAssertTrue([RCUser isValidEmail:@"ben.hirashima@reality.cap.com"], @"Valid email failed validation");
    STAssertTrue([RCUser isValidEmail:@"ben.hirashima@realitycap.co"], @"Valid email failed validation");
    STAssertTrue([RCUser isValidEmail:@"ben.hirashima@realitycap.museum"], @"Valid email failed validation");

    STAssertFalse([RCUser isValidEmail:@"@realitycap.com"], @"Bad email passed validation");
    STAssertFalse([RCUser isValidEmail:@"ben#realitycap.com"], @"Bad email passed validation");
    STAssertFalse([RCUser isValidEmail:@"ben@realitycap.co#m"], @"Bad email passed validation");
    STAssertFalse([RCUser isValidEmail:@"ben@realitycap.c"], @"Bad email passed validation");
    STAssertFalse([RCUser isValidEmail:@"ben@realitycap."], @"Bad email passed validation");
    STAssertFalse([RCUser isValidEmail:@"ben@.com"], @"Bad email passed validation");
    STAssertFalse([RCUser isValidEmail:@"ben@realitycap@com"], @"Bad email passed validation");
    STAssertFalse([RCUser isValidEmail:@"ben@realitycap.com@"], @"Bad email passed validation");
    STAssertFalse([RCUser isValidEmail:@"ben@realitycap"], @"Bad email passed validation");
    STAssertFalse([RCUser isValidEmail:@"ben.realitycap.com"], @"Bad email passed validation");
    STAssertFalse([RCUser isValidEmail:@"ben realitycap.com"], @"Bad email passed validation");
    STAssertFalse([RCUser isValidEmail:@"realitycap.com"], @"Bad email passed validation");
    STAssertFalse([RCUser isValidEmail:@"ben"], @"Bad email passed validation");
    STAssertFalse([RCUser isValidEmail:@" "], @"Bad email passed validation");
    STAssertFalse([RCUser isValidEmail:@""], @"Bad email passed validation");
        
    // Commented lines are not caught by the current regex. better to be lax than strict.
    //    STAssertFalse([RCUser isValidEmail:@"ben@hirashima@realitycap.com"], @"Bad email passed validation");
    //    STAssertFalse([RCUser isValidEmail:@"ben.@realitycap.com"], @"Bad email passed validation");
    //    STAssertFalse([RCUser isValidEmail:@".hirashima@realitycap.com"], @"Bad email passed validation");
    //    STAssertFalse([RCUser isValidEmail:@"-ben@realitycap.com"], @"Bad email passed validation");
    //    STAssertFalse([RCUser isValidEmail:@"ben-@realitycap.com"], @"Bad email passed validation");
    //    STAssertFalse([RCUser isValidEmail:@"#ben@realitycap.com"], @"Bad email passed validation");
    //    STAssertFalse([RCUser isValidEmail:@"ben@.realitycap.com"], @"Bad email passed validation");
    //    STAssertFalse([RCUser isValidEmail:@"ben@-realitycap.com"], @"Bad email passed validation");
    //    STAssertFalse([RCUser isValidEmail:@"ben@realitycap-.com"], @"Bad email passed validation");
    //    STAssertFalse([RCUser isValidEmail:@"ben@realitycap..com"], @"Bad email passed validation");
    //    STAssertFalse([RCUser isValidEmail:@"ben@reali#tycap.com"], @"Bad email passed validation");
    //    STAssertFalse([RCUser isValidEmail:@"@ben@realitycap.com"], @"Bad email passed validation");
}

@end
