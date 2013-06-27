//
//  RCUserManagerFactoryTests.m
//  RCCore
//
//  Created by Ben Hirashima on 2/5/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCUserManagerTests.h"
#import "RCUserManager.h"
#import "OCMock.h"
#import "RCUser.h"

#define VALID_EMAIL @"ben@realitycap.com"
#define VALID_PASSWORD @"secret"

@implementation RCUserManagerTests

- (BOOL)waitForCompletion:(NSTimeInterval)timeoutSecs {
    NSDate *timeoutDate = [NSDate dateWithTimeIntervalSinceNow:timeoutSecs];
    
    do {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:timeoutDate];
        if([timeoutDate timeIntervalSinceNow] < 0.0)
            break;
    } while (!done);
    
    return done;
}

- (void)setUp
{
    [super setUp];
    done = NO;
    [RCHTTPClient initWithBaseUrl:@"https://internal.realitycap.com/" withAcceptHeader:@"application/vnd.realitycap.json; version=1.0" withApiVersion:1];
}

- (void)tearDown
{
    [RCUser deleteStoredUser];
    [super tearDown];
}

- (void)testReturnsSameInstance
{
    RCUserManager* instance1 = [RCUserManager sharedInstance];
    RCUserManager* instance2 = [RCUserManager sharedInstance];
    
    STAssertEqualObjects(instance1, instance2, @"Get instance failed to return the same instance");
}

- (void)testHasValidStoredCredentials
{
    RCUser *user = [[RCUser alloc] init];
    user.username = VALID_EMAIL;
    user.password = VALID_PASSWORD;
    [user saveUser];
    
    STAssertTrue([[RCUserManager sharedInstance] hasValidStoredCredentials], @"hasValidStoredCredentials returned false but expected true");
}

- (void)testHasValidStoredCredentialsFailsWithZeroLengthUsername
{
    RCUser *user = [[RCUser alloc] init];
    user.username = @"";
    user.password = @"asdfasdf";
    [user saveUser];
    
    STAssertFalse([[RCUserManager sharedInstance] hasValidStoredCredentials], @"hasValidStoredCredentials returned true with zero length username");
}

- (void)testHasValidStoredCredentialsFailsWithZeroLengthPassword
{
    RCUser *user = [[RCUser alloc] init];
    user.username = @"asdfasdf";
    user.password = @"";
    [user saveUser];
    
    STAssertFalse([[RCUserManager sharedInstance] hasValidStoredCredentials], @"hasValidStoredCredentials returned true with zero length password");
}

- (void)testHasValidStoredCredentialsFailsWithNilUsername
{
    RCUser *user = [[RCUser alloc] init];
    user.password = @"asdfasdf";
    [user saveUser];
    
    STAssertFalse([[RCUserManager sharedInstance] hasValidStoredCredentials], @"hasValidStoredCredentials returned true with nil username");
}

- (void)testHasValidStoredCredentialsFailsWithNilPassword
{
    RCUser *user = [[RCUser alloc] init];
    user.username = @"asdfasdf";
    [user saveUser];
    
    STAssertFalse([[RCUserManager sharedInstance] hasValidStoredCredentials], @"hasValidStoredCredentials returned true with nil password");
}

- (void)testLogoutDeletesUser
{
    RCUser *user = [[RCUser alloc] init];
    user.username = VALID_EMAIL;
    user.password = VALID_PASSWORD;
    [user saveUser];
    
    [[RCUserManager sharedInstance] logout];
    STAssertFalse([[RCUserManager sharedInstance] hasValidStoredCredentials], @"Logout didn't delete stored user credentials");
}

- (void)testIsUsingAnonAccountTrue
{
    RCUser *user = [[RCUser alloc] init];
    user.username = @"asdfasdf";
    user.password = @"supersecret";
    [user saveUser];
    
    STAssertTrue([[RCUserManager sharedInstance] isUsingAnonAccount], @"isUsingAnonAccount returned false but expected true");
}

- (void)testIsUsingAnonAccountFalse
{
    RCUser *user = [[RCUser alloc] init];
    user.username = VALID_EMAIL;
    user.password = VALID_PASSWORD;
    [user saveUser];
    
    STAssertFalse([[RCUserManager sharedInstance] isUsingAnonAccount], @"isUsingAnonAccount returned true but expected false");
}

- (void)testFetchSessionCookie
{
    RCUserManager* userMan = [RCUserManager sharedInstance];
    
    [userMan
     fetchSessionCookie:^(NSHTTPCookie *cookie)
     {
         STAssertTrue([cookie.name isEqualToString:@"csrftoken"], @"CSRF cookie not returned to success block");
         STAssertTrue(cookie.value.length, @"CSRF cookie value is zero length");
         done = YES;
     }
     onFailure:^(int statusCode)
     {
         STFail(@"fetchSessionCookie called failure block with status code %i", statusCode);
         done = YES;
     }];
    
    STAssertTrue([self waitForCompletion:10.0], @"Request timed out");
}

- (void)testCreateAnonAccount
{
    RCUserManager* userMan = [RCUserManager sharedInstance];
    
    [userMan
     createAnonAccount:^(NSString *username)
     {
         STAssertTrue(username.length, @"Username is zero length");
         done = YES;
     }
     onFailure:^(int statusCode)
     {
         STFail(@"createAnonAccount called failure block with status code %i", statusCode);
         done = YES;
     }];
    
    STAssertTrue([self waitForCompletion:10.0], @"Request timed out");
}

- (void)testLoginWithUsernameAndPassword
{
    RCUserManager* userMan = [RCUserManager sharedInstance];
    
    [userMan
     loginWithUsername:VALID_EMAIL
     withPassword:VALID_PASSWORD
     onSuccess:^()
     {
         done = YES;
     }
     onFailure:^(int statusCode)
     {
         STFail(@"loginWithUsername:withPassword called failure block with status code %i", statusCode);
         done = YES;
     }];
    
    STAssertTrue([self waitForCompletion:10.0], @"Request timed out");
}

- (void)testLoginWithStoredCredentials
{
    RCUser *user = [[RCUser alloc] init];
    user.username = VALID_EMAIL;
    user.password = VALID_PASSWORD;
    [user saveUser];
    
    RCUserManager* userMan = [RCUserManager sharedInstance];
    
    [userMan
     loginWithStoredCredentials:^()
     {
         done = YES;
     }
     onFailure:^(int statusCode)
     {
         STFail(@"loginWithStoredCredentials called failure block with status code %i", statusCode);
         done = YES;
     }];
    
    STAssertTrue([self waitForCompletion:10.0], @"Request timed out");
}

- (void)testCreateAnonAccountThenUpdateUserThenLogin
{
    RCUserManager* userMan = [RCUserManager sharedInstance];
    
    [userMan
     createAnonAccount:^(NSString *username)
     {
         STAssertTrue(username.length, @"Username is zero length");
         
         [userMan
          loginWithStoredCredentials:^()
          {
              RCUser *user = [RCUser getStoredUser];
              user.firstName = @"Ben";
              user.lastName = @"Test";
              [user saveUser];
              
              [userMan
               updateUser:user
               onSuccess:^()
               {
                   [userMan
                    loginWithStoredCredentials:^()
                    {
                        done = YES;
                    }
                    onFailure:^(int statusCode)
                    {
                        STFail(@"loginWithStoredCredentials called failure block with status code %i", statusCode);
                        done = YES;
                    }];
               }
               onFailure:^(int statusCode)
               {
                   STFail(@"updateUser called failure block with status code %i", statusCode);
                   done = YES;
               }];
          }
          onFailure:^(int statusCode)
          {
              STFail(@"loginWithStoredCredentials called failure block with status code %i", statusCode);
              done = YES;
          }];
     }
     onFailure:^(int statusCode)
     {
         STFail(@"createAnonAccount called failure block with status code %i", statusCode);
         done = YES;
     }];
    
    STAssertTrue([self waitForCompletion:10.0], @"Request timed out");
}

- (void)testInvalidCredentialsResultsInLoginStateError
{
    RCUser *user = [[RCUser alloc] init];
    user.username = @"asdfasdf";
    user.password = @"";
    [user saveUser];
    
    [[RCUserManager sharedInstance]
     loginWithStoredCredentials:^
     {
         STFail(@"Successful login with invalid credentials");
     }
     onFailure:^(int statusCode)
     {
         STAssertEquals([[RCUserManager sharedInstance] getLoginState], LoginStateError, @"Expected LoginStateError");
     }];
}
@end
