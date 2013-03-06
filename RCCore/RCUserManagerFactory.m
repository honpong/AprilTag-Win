//
//  RCUserManagerFactory.m
//  RCCore
//
//  Created by Ben Hirashima on 2/5/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCUserManagerFactory.h"

static const NSString *USERNAME_PARAM = @"username";
static const NSString *PASSWORD_PARAM = @"password";
static const NSString *CSRF_TOKEN_COOKIE = @"csrftoken";
static const NSString *CSRF_TOKEN_PARAM = @"csrfmiddlewaretoken";
static const NSString *EMAIL_PARAM = @"email";
static const NSString *FIRST_NAME_PARAM = @"first_name";
static const NSString *LAST_NAME_PARAM = @"last_name";

static const int USERNAME_MAX_LENGTH = 30;
static const int PASSWORD_MAX_LENGTH = 30;

@interface RCUserManagerImpl : NSObject <RCUserManager>
{
    BOOL _isLoggedIn;
    NSHTTPCookie *csrfCookie;
}
@end

@implementation RCUserManagerImpl

- (id)init
{
    self = [super init];
    
    if (self)
    {
        _isLoggedIn = NO;
    }
    
    return self;
}

- (void) fetchSessionCookie:(void (^)(NSHTTPCookie *cookie))successBlock onFailure:(void (^)(int))failureBlock
{
    AFHTTPClient *client = [RCHttpClientFactory getInstance];
    if (client == nil)
    {
        NSLog(@"Http client is nil");
        failureBlock(0);
        return;
    }
    
    [client getPath:@"accounts/login/"
         parameters:nil
            success:^(AFHTTPRequestOperation *operation, id JSON)
            {
                NSLog(@"Fetched cookies");
                
                NSArray *cookies = [[NSHTTPCookieStorage sharedHTTPCookieStorage] cookiesForURL:operation.request.URL];
                
                for (NSHTTPCookie *cookie in cookies)
                {
                    if ([cookie.name isEqual:CSRF_TOKEN_COOKIE])
                    {
                        csrfCookie = cookie;
                        NSLog(@"%@:%@", csrfCookie.name, csrfCookie.value);
                        if (successBlock) successBlock(csrfCookie);
                    }
                }
                
                if (csrfCookie == nil)
                {
                    NSLog(@"CSRF cookie not found in response");
                    if (failureBlock) failureBlock(0);
                }
            }
            failure:^(AFHTTPRequestOperation *operation, NSError *error)
            {
                NSLog(@"Failed to fetch cookie: %i", operation.response.statusCode);

                if (failureBlock) failureBlock(operation.response.statusCode);
            }
     ];
}

- (BOOL) hasValidStoredCredentials
{
//    return NO; //for testing
    RCUser *user = [RCUser getStoredUser];
    return [self areCredentialsValid:user.username withPassword:user.password];
}

- (BOOL)isUsingAnonAccount
{
    RCUser *user = [RCUser getStoredUser];
    BOOL isAnon = [user.username containsString:@"@"];
    return isAnon;
}

- (BOOL) areCredentialsValid:(NSString*)username withPassword:(NSString*)password
{
    return username && password && username.length && password.length && username.length <= USERNAME_MAX_LENGTH && password.length <= PASSWORD_MAX_LENGTH;
}

- (void) loginWithStoredCredentials:(void (^)())successBlock onFailure:(void (^)(int))failureBlock
{
    RCUser *user = [RCUser getStoredUser];
    
    if ([self areCredentialsValid:user.username withPassword:user.password])
    {
        [self loginWithUsername:user.username withPassword:user.password onSuccess:successBlock onFailure:failureBlock];
    }
    else
    {
        NSLog(@"Invalid stored login credentials");
        failureBlock(0); //zero means special non-http error
    }
}

- (void) loginWithUsername:(NSString *)username
             withPassword:(NSString *)password
                onSuccess:(void (^)())successBlock
                onFailure:(void (^)(int))failureBlock
{
    [self
     fetchSessionCookie:^(NSHTTPCookie *cookie)
     {
         [self postLoginWithUsername:username withPassword:password onSuccess:successBlock onFailure:failureBlock];
     }
     onFailure:^(int statusCode)
     {
         if (failureBlock) failureBlock(statusCode);
     }
     ];
}

- (void) postLoginWithUsername:(NSString *)username
                  withPassword:(NSString *)password
                     onSuccess:(void (^)())successBlock
                     onFailure:(void (^)(int))failureBlock
{
    NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:
                            username, USERNAME_PARAM,
                            password, PASSWORD_PARAM,
                            csrfCookie.value, CSRF_TOKEN_PARAM,
                            nil];
    
    AFHTTPClient *client = [RCHttpClientFactory getInstance];
    if (client == nil)
    {
        NSLog(@"Http client is nil");
        failureBlock(0);
        return;
    }
    
    //workaround for django weirdness. referer is required or login doesn't work.
    [client setDefaultHeader:@"Referer" value:@"https://internal.realitycap.com/accounts/login/"];
    
    [client postPath:@"accounts/login/"
          parameters:params
             success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         if ([operation.responseString containsString:@"Successfully Logged In"])
         {
             NSLog(@"Logged in as %@", username);
             _isLoggedIn = YES;
             if (successBlock) successBlock();
         }
         else
         {
             NSLog(@"Login failure: %i %@", operation.response.statusCode, operation.responseString);
             _isLoggedIn = NO;
             if (failureBlock) failureBlock(operation.response.statusCode);
         }
     }
             failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         NSLog(@"Login failure: %i %@", operation.response.statusCode, operation.responseString);
         _isLoggedIn = NO;
         if (failureBlock) failureBlock(operation.response.statusCode);
     }
     ];
    
    //workaround for django weirdness. see above.
    [client setDefaultHeader:@"Referer" value:@""];
}

- (void) logout
{
    [RCUser deleteStoredUser];
    _isLoggedIn = NO;
}

- (BOOL) isLoggedIn
{
    return _isLoggedIn;
}

- (void) createAccount:(RCUser*)user
             onSuccess:(void (^)(int userId))successBlock
             onFailure:(void (^)(int statusCode))failureBlock
{
    NSLog(@"Create account...");
    
    NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:
                            user.username, USERNAME_PARAM,
                            user.password, PASSWORD_PARAM,
                            @"", EMAIL_PARAM,
                            user.firstName ? user.firstName : @"", FIRST_NAME_PARAM,
                            user.lastName ? user.lastName : @"", LAST_NAME_PARAM,
                            nil];
    
    AFHTTPClient *client = [RCHttpClientFactory getInstance];
    if (client == nil)
    {
        NSLog(@"Http client is nil");
        failureBlock(0);
        return;
    }
    
    [client postPath:@"api/users/"
          parameters:params
             success:^(AFHTTPRequestOperation *operation, id JSON)
             {
                 NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:nil];//TODO:handle error
                 
                 id userId = [response objectForKey:@"id"];
                 
                 if ([userId isKindOfClass:[NSNumber class]] && [userId integerValue] > 0)
                 {
                    NSLog(@"Account created\nusername: %@\npassword: %@\nuser id: %i", user.username, user.password, [userId intValue]);
                     
                     user.dbid = userId;
                     [user saveUser];
                     
                     if (successBlock) successBlock(user.dbid);
                 }
                 else
                 {
                     NSLog(@"Failed to create account. Unable to find user ID in response");
                     if (failureBlock) failureBlock(0);
                 }
             }
             failure:^(AFHTTPRequestOperation *operation, NSError *error)
             {
                 NSLog(@"Failed to create account: %i %@", operation.response.statusCode, operation.responseString);
                 
                 if (failureBlock) failureBlock(operation.response.statusCode);
             }
     ];
}

- (void) updateUser:(RCUser*)user
              onSuccess:(void (^)())successBlock
              onFailure:(void (^)(int))failureBlock
{
    NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:
                            user.username,  USERNAME_PARAM,
                            user.password,  PASSWORD_PARAM,
                            user.username,  EMAIL_PARAM,
                            user.firstName, FIRST_NAME_PARAM,
                            user.lastName,  LAST_NAME_PARAM,
                            nil];
    
    AFHTTPClient *client = [RCHttpClientFactory getInstance];
    if (client == nil)
    {
        NSLog(@"Http client is nil");
        failureBlock(0);
        return;
    }
    
    NSString *url = [NSString stringWithFormat:@"api/user/%i/", [user.dbid integerValue]];
    
    NSLog(@"Update user...");
//    NSLog(@"PUT %@", url);
//    NSLog(@"params: \n%@", params);
    
    [client putPath:url
         parameters:params
            success:^(AFHTTPRequestOperation *operation, id JSON)
            {
                NSLog(@"User modified");
                if (successBlock) successBlock();
            }
            failure:^(AFHTTPRequestOperation *operation, NSError *error)
            {
                NSLog(@"Failed to modify user: %i\n%@", operation.response.statusCode, operation.responseString);

                if (failureBlock) failureBlock(operation.response.statusCode);
            }
     ];
}

- (void) createAnonAccount:(void (^)(NSString* username))successBlock onFailure:(void (^)(int))failureBlock
{
    NSLog(@"Create anon account...");
    
    RCUser *user = [self generateNewAnonUser];
    
    [self
     createAccount:user
     onSuccess:^(int userId)
     {
         if (successBlock) successBlock(user.username);
     }
     onFailure:^(int statusCode)
     {
         if (failureBlock) failureBlock(statusCode);
     }
    ];
}

- (RCUser*)generateNewAnonUser
{
    RCUser *user = [[RCUser alloc] init];
    user.username = [[[Guid randomGuid] stringValueWithFormat:GuidFormatCompact] substringToIndex:USERNAME_MAX_LENGTH - 2];
    user.password = [[[Guid randomGuid] stringValueWithFormat:GuidFormatCompact] substringToIndex:PASSWORD_MAX_LENGTH - 2];
    return user;
}

@end

@implementation RCUserManagerFactory

static id<RCUserManager> instance;

+ (id<RCUserManager>) getInstance
{
    if (instance == nil)
    {
        instance = [[RCUserManagerImpl alloc] init];
    }
    
    return instance;
}

+ (void) setInstance:(id<RCUserManager>)mockObject
{
    instance = mockObject;
}

@end
