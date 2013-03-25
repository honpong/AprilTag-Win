//
//  RCUserManagerFactoryTests.m
//  RCCore
//
//  Created by Ben Hirashima on 2/5/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCUserManagerFactoryTests.h"
#import "RCUserManagerFactory.h"
#import "OCMock.h"
#import "RCUser.h"

@implementation RCUserManagerFactoryTests

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
    [RCHttpClientFactory initWithBaseUrl:@"https://internal.realitycap.com/" withAcceptHeader:@"application/vnd.realitycap.json; version=1.0" withApiVersion:1];
}

- (void)tearDown
{
    [RCUserManagerFactory setInstance:nil];
    [RCHttpClientFactory setInstance:nil];
    [RCUser deleteStoredUser];
    
    [super tearDown];
}

- (void)testReturnsSameInstance
{
    id<RCUserManager> instance1 = [RCUserManagerFactory getInstance];
    id<RCUserManager> instance2 = [RCUserManagerFactory getInstance];
    
    STAssertEqualObjects(instance1, instance2, @"Get instance failed to return the same instance");
}

- (void)testSetInstance
{
    id<RCUserManager> instance1 = [OCMockObject mockForProtocol:@protocol(RCUserManager)];
    
    [RCUserManagerFactory setInstance:instance1];
    
    id<RCUserManager> instance2 = [RCUserManagerFactory getInstance];
    
    STAssertEqualObjects(instance1, instance2, @"Get instance failed to return the same instance after set instance was called");
}

- (void)testHasValidStoredCredentials
{
    RCUser *user = [[RCUser alloc] init];
    user.username = @"ben@realitycap.com";
    user.password = @"secret";
    [user saveUser];
    
    STAssertTrue([[RCUserManagerFactory getInstance] hasValidStoredCredentials], @"hasValidStoredCredentials returned false but expected true");
}

- (void)testHasValidStoredCredentialsFailsWithZeroLengthUsername
{
    RCUser *user = [[RCUser alloc] init];
    user.username = @"";
    user.password = @"asdfasdf";
    [user saveUser];
    
    STAssertFalse([[RCUserManagerFactory getInstance] hasValidStoredCredentials], @"hasValidStoredCredentials returned true with zero length username");
}

- (void)testHasValidStoredCredentialsFailsWithZeroLengthPassword
{
    RCUser *user = [[RCUser alloc] init];
    user.username = @"asdfasdf";
    user.password = @"";
    [user saveUser];
    
    STAssertFalse([[RCUserManagerFactory getInstance] hasValidStoredCredentials], @"hasValidStoredCredentials returned true with zero length password");
}

- (void)testHasValidStoredCredentialsFailsWithNilUsername
{
    RCUser *user = [[RCUser alloc] init];
    user.password = @"asdfasdf";
    [user saveUser];
    
    STAssertFalse([[RCUserManagerFactory getInstance] hasValidStoredCredentials], @"hasValidStoredCredentials returned true with nil username");
}

- (void)testHasValidStoredCredentialsFailsWithNilPassword
{
    RCUser *user = [[RCUser alloc] init];
    user.username = @"asdfasdf";
    [user saveUser];
    
    STAssertFalse([[RCUserManagerFactory getInstance] hasValidStoredCredentials], @"hasValidStoredCredentials returned true with nil password");
}

- (void)testLogoutDeletesUser
{
    RCUser *user = [[RCUser alloc] init];
    user.username = @"ben@realitycap.com";
    user.password = @"secret";
    [user saveUser];
    
    [[RCUserManagerFactory getInstance] logout];
    STAssertFalse([[RCUserManagerFactory getInstance] hasValidStoredCredentials], @"Logout didn't delete stored user credentials");
}

- (void)testIsUsingAnonAccount
{
    RCUser *user = [[RCUser alloc] init];
    user.username = @"asdfasdf";
    user.password = @"supersecret";
    [user saveUser];
    
    STAssertTrue([[RCUserManagerFactory getInstance] isUsingAnonAccount], @"isUsingAnonAccount returned false but expected true");
}

- (void)testIsUsingAnonAccountFails
{
    RCUser *user = [[RCUser alloc] init];
    user.username = @"ben@realitycap.com";
    user.password = @"secret";
    [user saveUser];
    
    STAssertFalse([[RCUserManagerFactory getInstance] isUsingAnonAccount], @"isUsingAnonAccount returned true but expected false");
}

- (void)testFetchSessionCookie
{
    id<RCUserManager> userMan = [RCUserManagerFactory getInstance];
    
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
    id<RCUserManager> userMan = [RCUserManagerFactory getInstance];
    
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
    id<RCUserManager> userMan = [RCUserManagerFactory getInstance];
    
    [userMan
     loginWithUsername:@"ben@realitycap.com"
     withPassword:@"secret"
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
    user.username = @"ben@realitycap.com";
    user.password = @"secret";
    [user saveUser];
    
    id<RCUserManager> userMan = [RCUserManagerFactory getInstance];
    
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
    id<RCUserManager> userMan = [RCUserManagerFactory getInstance];
    
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
    
    [[RCUserManagerFactory getInstance]
     loginWithStoredCredentials:^
     {
         STFail(@"Successful login with invalid credentials");
     }
     onFailure:^(int statusCode)
     {
         STAssertEquals([[RCUserManagerFactory getInstance] getLoginState], LoginStateError, @"Expected LoginStateError");
     }];
}
@end
