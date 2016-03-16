//
//  RCUserManager.m
//  RCCore
//
//  Created by Ben Hirashima on 2/5/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCUserManager.h"
#import "RCConstants.h"

static const NSString *USERNAME_PARAM = @"username";
static const NSString *PASSWORD_PARAM = @"password";
static const NSString *CSRF_TOKEN_COOKIE = @"csrftoken";
static const NSString *CSRF_TOKEN_PARAM = @"csrfmiddlewaretoken";
static const NSString *EMAIL_PARAM = @"email";
static const NSString *FIRST_NAME_PARAM = @"first_name";
static const NSString *LAST_NAME_PARAM = @"last_name";

@implementation RCUserManager
{
    LoginState _loginState;
    NSHTTPCookie *csrfCookie;
}

+ (RCUserManager *) sharedInstance
{
    static RCUserManager * instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[self alloc] init];
    });
    return instance;
}

- (id)init
{
    self = [super init];
    
    if (self)
    {
        _loginState = LoginStateNo;
    }
    
    return self;
}

- (void) fetchSessionCookie:(void (^)(NSHTTPCookie *cookie))successBlock onFailure:(void (^)(NSInteger))failureBlock
{
    AFHTTPClient *client = [RCHTTPClient sharedInstance];
    if (client == nil)
    {
        DLog(@"Http client is nil");
        failureBlock(0);
        return;
    }
    
    [client getPath:@"accounts/login/"
         parameters:nil
            success:^(AFHTTPRequestOperation *operation, id JSON)
            {
                DLog(@"Fetched cookies");
                
                NSArray *cookies = [[NSHTTPCookieStorage sharedHTTPCookieStorage] cookiesForURL:operation.request.URL];
                
                for (NSHTTPCookie *cookie in cookies)
                {
                    if ([cookie.name isEqual:CSRF_TOKEN_COOKIE])
                    {
                        csrfCookie = cookie;
                        DLog(@"%@:%@", csrfCookie.name, csrfCookie.value);
                        if (successBlock) successBlock(csrfCookie);
                    }
                }
                
                if (csrfCookie == nil)
                {
                    DLog(@"CSRF cookie not found in response");
                    if (failureBlock) failureBlock(0);
                }
            }
            failure:^(AFHTTPRequestOperation *operation, NSError *error)
            {
                DLog(@"Failed to fetch cookie: %li", (long)operation.response.statusCode);

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
    BOOL isAnon = ![user.username containsString:@"@"];
    return isAnon;
}

- (BOOL) areCredentialsValid:(NSString*)username withPassword:(NSString*)password
{
    BOOL valid = YES;
    valid = valid && username;
    valid = valid && password;
    valid = valid && username.length;
    valid = valid && password.length;
    
    return valid;
}

- (void) loginWithStoredCredentials:(void (^)())successBlock onFailure:(void (^)(NSInteger))failureBlock
{
    RCUser *user = [RCUser getStoredUser];
    
    if ([self areCredentialsValid:user.username withPassword:user.password])
    {
        [self loginWithUsername:user.username withPassword:user.password onSuccess:successBlock onFailure:failureBlock];
    }
    else
    {
        DLog(@"Invalid stored login credentials");
        _loginState = LoginStateError;
        failureBlock(0); //zero means special non-http error
    }
}

- (void) loginWithUsername:(NSString *)username
             withPassword:(NSString *)password
                onSuccess:(void (^)())successBlock
                onFailure:(void (^)(NSInteger))failureBlock
{
    [self
     fetchSessionCookie:^(NSHTTPCookie *cookie)
     {
         [self postLoginWithUsername:username withPassword:password onSuccess:successBlock onFailure:failureBlock];
     }
     onFailure:^(NSInteger statusCode)
     {
         _loginState = LoginStateError;
         if (failureBlock) failureBlock(statusCode);
     }
     ];
}

- (void) postLoginWithUsername:(NSString *)username
                  withPassword:(NSString *)password
                     onSuccess:(void (^)())successBlock
                     onFailure:(void (^)(NSInteger))failureBlock
{
    NSDictionary *params = @{EMAIL_PARAM: username,
                            PASSWORD_PARAM: password,
                            CSRF_TOKEN_PARAM: csrfCookie.value};
    
    AFHTTPClient *client = [RCHTTPClient sharedInstance];
    if (client == nil)
    {
        DLog(@"Http client is nil");
        failureBlock(0);
        return;
    }
    
    [client
     postPath:@"api/v1/login/"
     parameters:params
     success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         DLog(@"Logged in as %@", username);
         _loginState = LoginStateYes;
         if (successBlock) successBlock();
     }
     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         DLog(@"Login failure: %li %@", (long)operation.response.statusCode, operation.responseString);
         _loginState = LoginStateError;
         if (failureBlock) failureBlock(operation.response.statusCode);
     }
     ];
    
    //workaround for django weirdness. see above.
    [client setDefaultHeader:@"Referer" value:@""];
}

- (void) logout
{
    [RCUser deleteStoredUser];
    _loginState = LoginStateNo;
}

- (LoginState) getLoginState
{
    return _loginState;
}

- (void) createAccount:(RCUser*)user
             onSuccess:(void (^)(int userId))successBlock
             onFailure:(void (^)(NSInteger statusCode))failureBlock
{
    LOGME
    
    NSDictionary *params = @{USERNAME_PARAM: user.username,
                            PASSWORD_PARAM: user.password,
                            EMAIL_PARAM: user.username,
                            FIRST_NAME_PARAM: user.firstName ? user.firstName : @"",
                            LAST_NAME_PARAM: user.lastName ? user.lastName : @""};
    
    RCHTTPClient *client = [RCHTTPClient sharedInstance];
    if (client == nil)
    {
        DLog(@"Http client is nil");
        failureBlock(0);
        return;
    }
    
    NSString *path = [NSString stringWithFormat:@"api/v%i/users/", client.apiVersion];
    
    DLog(@"POST %@\n%@", path, params);
    
    [client postPath:path
          parameters:params
             success:^(AFHTTPRequestOperation *operation, id JSON)
             {
                 NSError* error;
                 NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:&error];
                 if (error)
                 {
                     DLog(@"Failed to parse response JSON.");
                     if (failureBlock) failureBlock(0);
                 }
                 
                 id userId = response[@"id"];
                 
                 if ([userId isKindOfClass:[NSNumber class]] && [userId integerValue] > 0)
                 {
                    DLog(@"Account created\nusername: %@\npassword: %@\nuser id: %i", user.username, user.password, [userId intValue]);
                     
                     user.dbid = userId;
                     [user saveUser];
                     
                     if (successBlock) successBlock(user.dbid);
                 }
                 else
                 {
                     DLog(@"Failed to create account. Unable to find user ID in response");
                     if (failureBlock) failureBlock(0);
                 }
             }
             failure:^(AFHTTPRequestOperation *operation, NSError *error)
             {
                 DLog(@"Failed to create account: %li %@", (long)operation.response.statusCode, operation.responseString);
                 
                 if (failureBlock) failureBlock(operation.response.statusCode);
             }
     ];
}

- (void) updateUser:(RCUser*)user
              onSuccess:(void (^)())successBlock
              onFailure:(void (^)(NSInteger))failureBlock
{
    NSDictionary *params = @{USERNAME_PARAM: user.username,
                            PASSWORD_PARAM: user.password,
                            EMAIL_PARAM: user.username,
                            FIRST_NAME_PARAM: user.firstName,
                            LAST_NAME_PARAM: user.lastName};
    
    RCHTTPClient *client = [RCHTTPClient sharedInstance];
    if (client == nil)
    {
        DLog(@"Http client is nil");
        failureBlock(0);
        return;
    }
    
    NSString *path = [NSString stringWithFormat:@"api/v%i/user/%li/", client.apiVersion, (long)[user.dbid integerValue]];
    
    DLog(@"PUT %@\n%@", path, params);
    
    [client putPath:path
         parameters:params
            success:^(AFHTTPRequestOperation *operation, id JSON)
            {
                DLog(@"User modified");
                if (successBlock) successBlock();
            }
            failure:^(AFHTTPRequestOperation *operation, NSError *error)
            {
                DLog(@"Failed to modify user: %li\n%@", (long)operation.response.statusCode, operation.responseString);

                if (failureBlock) failureBlock(operation.response.statusCode);
            }
     ];
}

- (void) createAnonAccount:(void (^)(NSString* username))successBlock onFailure:(void (^)(NSInteger))failureBlock
{
    LOGME
    
    RCUser *user = [self generateNewAnonUser];
    
    [self
     createAccount:user
     onSuccess:^(int userId)
     {
         if (successBlock) successBlock(user.username);
     }
     onFailure:^(NSInteger statusCode)
     {
         if (failureBlock) failureBlock(statusCode);
     }
    ];
}

- (RCUser*)generateNewAnonUser
{
    RCUser *user = [[RCUser alloc] init];
    user.username = [[Guid randomGuid] stringValueWithFormat:GuidFormatCompact];
    user.password = [[Guid randomGuid] stringValueWithFormat:GuidFormatCompact];
    return user;
}

@end
