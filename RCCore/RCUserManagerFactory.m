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

- (void) fetchSessionCookie:(void (^)())successBlock onFailure:(void (^)(int))failureBlock
{
    AFHTTPClient *client = [RCHttpClientFactory getInstance];
    
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
                        if (successBlock) successBlock();
                    }
                }
                
                if (csrfCookie == nil && failureBlock)
                {
                    NSLog(@"CSRF cookie not found in response");
                    failureBlock(0);
                }
            }
            failure:^(AFHTTPRequestOperation *operation, NSError *error)
            {
                NSLog(@"Failed to fetch cookie: %i", operation.response.statusCode);

                if (failureBlock) failureBlock(operation.response.statusCode);
            }
     ];
}

- (BOOL) hasStoredCredentials
{
//    return NO; //for testing
    NSString *username = [[NSUserDefaults standardUserDefaults] objectForKey:PREF_USERNAME];
    NSString *password = [[NSUserDefaults standardUserDefaults] objectForKey:PREF_PASSWORD];
    
    return [self areCredentialsValid:username withPassword:password];
}

- (BOOL) areCredentialsValid:(NSString*)username withPassword:(NSString*)password
{
    return username && password && username.length && password.length && username.length <= USERNAME_MAX_LENGTH && password.length <= PASSWORD_MAX_LENGTH;
}

- (void) loginWithStoredCredentials:(void (^)())successBlock onFailure:(void (^)(int))failureBlock
{
    NSString *username = [[NSUserDefaults standardUserDefaults] objectForKey:PREF_USERNAME];
    NSString *password = [[NSUserDefaults standardUserDefaults] objectForKey:PREF_PASSWORD];
    
    if ([self areCredentialsValid:username withPassword:password])
    {
        [self loginWithUsername:username withPassword:password onSuccess:successBlock onFailure:failureBlock];
    }
    else
    {
        NSLog(@"Invalid stored login credentials");
        failureBlock(0);
    }
}

- (void) loginWithUsername:(NSString *)username
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
    
    [client postPath:@"accounts/login/"
          parameters:params
             success:^(AFHTTPRequestOperation *operation, id JSON)
             {
                 NSLog(@"Logged in");
                 _isLoggedIn = YES;
                 
                 if (successBlock) successBlock();
             }
             failure:^(AFHTTPRequestOperation *operation, NSError *error)
             {
                 NSLog(@"Login failure: %i", operation.response.statusCode);
                 
                 if (failureBlock) failureBlock(operation.response.statusCode);
                 
                 _isLoggedIn = NO;
             }
    ];
}

- (void) logout
{
    _isLoggedIn = NO;
}

- (BOOL) isLoggedIn
{
    return _isLoggedIn;
}

- (void) createAccount:(RCUser*)user
             onSuccess:(void (^)())successBlock
             onFailure:(void (^)(int))failureBlock
{
    NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:
                            user.username, USERNAME_PARAM,
                            user.password, PASSWORD_PARAM,
                            user.email ? user.email : @"", EMAIL_PARAM,
                            user.firstName ? user.firstName : @"", FIRST_NAME_PARAM,
                            user.lastName ? user.lastName : @"", LAST_NAME_PARAM,
                            nil];
    
    AFHTTPClient *client = [RCHttpClientFactory getInstance];
    
    [client postPath:@"api/users/"
          parameters:params
             success:^(AFHTTPRequestOperation *operation, id JSON)
             {
                 NSLog(@"Account created");
                 
                 if (successBlock) successBlock();
             }
             failure:^(AFHTTPRequestOperation *operation, NSError *error)
             {
                 NSLog(@"Failed to create account: %i", operation.response.statusCode);
                 
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
                            user.email,     EMAIL_PARAM,
                            user.firstName, FIRST_NAME_PARAM,
                            user.lastName,  LAST_NAME_PARAM,
                            nil];
    
    AFHTTPClient *client = [RCHttpClientFactory getInstance];
    
    [client putPath:@"api/users/"
         parameters:params
            success:^(AFHTTPRequestOperation *operation, id JSON)
            {
                NSLog(@"User modified");
                              
                if (successBlock) successBlock();
            }
            failure:^(AFHTTPRequestOperation *operation, NSError *error)
            {
                NSLog(@"Failed to modify user: %i", operation.response.statusCode);

                if (failureBlock) failureBlock(operation.response.statusCode);
            }
     ];
}

- (void) createAnonAccount:(void (^)(NSString* username))successBlock onFailure:(void (^)(int))failureBlock
{
    RCUser *user = [[RCUser alloc] init];
    user.username = [[[Guid randomGuid] stringValueWithFormat:GuidFormatCompact] substringToIndex:USERNAME_MAX_LENGTH - 2];
    user.password = [[[Guid randomGuid] stringValueWithFormat:GuidFormatCompact] substringToIndex:PASSWORD_MAX_LENGTH - 2];
    
    [self
     createAccount:user
     onSuccess:^()
     {
         NSLog(@"Anon account created\nusername: %@\npassword: %@", user.username, user.password);
         [self saveCredentials:user.username withPassword:user.password];
         if (successBlock) successBlock(user.username);
     }
     onFailure:^(int statusCode)
     {
         if (failureBlock) failureBlock(statusCode);
     }
    ];
}

- (void) saveCredentials:(NSString*)username withPassword:(NSString*)password
{
    [[NSUserDefaults standardUserDefaults] setObject:username forKey:PREF_USERNAME];
    [[NSUserDefaults standardUserDefaults] setObject:password forKey:PREF_PASSWORD]; //TODO:store securely in keychain
    [[NSUserDefaults standardUserDefaults] synchronize];
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
