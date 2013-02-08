//
//  RCUserManagerFactory.m
//  RCCore
//
//  Created by Ben Hirashima on 2/5/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCUserManagerFactory.h"

@interface RCUserManagerImpl : NSObject <RCUserManager>
{
    BOOL _isLoggedIn;
    AFHTTPClient *client;
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
        
        client = [RCHttpClientFactory getInstance];
        
        if (client == nil) NSLog(@"Warning: HTTP client is nil");
    }
    
    return self;
}

- (void)fetchSessionCookie:(void (^)())successBlock onFailure:(void (^)(int))failureBlock
{
    [client getPath:@"accounts/login/"
         parameters:nil
            success:^(AFHTTPRequestOperation *operation, id JSON)
            {
                NSLog(@"Fetched cookies");
                
                NSArray *cookies = [[NSHTTPCookieStorage sharedHTTPCookieStorage] cookiesForURL:operation.request.URL];
                
                for (NSHTTPCookie *cookie in cookies)
                {
                    if ([cookie.name isEqualToString:@"csrftoken"]) csrfCookie = cookie;
                    NSLog(@"%@:%@", cookie.name, cookie.value);
                }
                
                if (successBlock) successBlock();
            }
            failure:^(AFHTTPRequestOperation *operation, NSError *error)
            {
                NSLog(@"Failed to fetch cookie: %i", operation.response.statusCode);

                if (failureBlock) failureBlock(operation.response.statusCode);
            }
     ];
}

- (void)loginWithUsername:(NSString *)username
             withPassword:(NSString *)password
                onSuccess:(void (^)())successBlock
                onFailure:(void (^)(int))failureBlock
{
    NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:username, @"username", password, @"password", csrfCookie.value, @"csrfmiddlewaretoken", nil];
    
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

- (void)logout
{
    _isLoggedIn = NO;
}

- (BOOL)isLoggedIn
{
    return _isLoggedIn;
}

- (void)createAccount:(NSString*)username
         withPassword:(NSString*)password
            onSuccess:(void (^)())successBlock
            onFailure:(void (^)(int))failureBlock
{
    NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:username, @"username", password, @"password", nil];
    
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

- (void)changeUsername:(NSString*)username
           andPassword:(NSString*)password
             onSuccess:(void (^)())successBlock
             onFailure:(void (^)(int))failureBlock
{
    NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:username, @"username", password, @"password", nil];
    
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

@end

@implementation RCUserManagerFactory

static id<RCUserManager> instance;

+ (id<RCUserManager>)getInstance
{
    if (instance == nil)
    {
        instance = [[RCUserManagerImpl alloc] init];
    }
    
    return instance;
}

+ (void)setInstance:(id<RCUserManager>)mockObject
{
    instance = mockObject;
}

@end
