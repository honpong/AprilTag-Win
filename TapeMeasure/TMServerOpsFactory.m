//
//  TMServerOpsFactory.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMServerOpsFactory.h"

@interface TMServerOpsImpl : NSObject <TMServerOps>
{
    
}
@end

@implementation TMServerOpsImpl

- (id)init
{
    self = [super init];
    
    if (self)
    {
        
    }
    
    return self;
}

- (void) createAnonAccount: (void (^)())successBlock onFailure: (void (^)())failureBlock
{
    [USER_MANAGER
     createAnonAccount:^(NSString* username)
     {
         [Flurry logEvent:@"User.AnonAccountCreated"];
         if (successBlock) successBlock();
     }
     onFailure:^(int statusCode)
     {
         NSLog(@"createAnonAccount failure callback:%i", statusCode);
         [Flurry
          logError:@"HTTP.CreateAnonAccount"
          message:[NSString stringWithFormat:@"%i", statusCode]
          error:nil
          ];
         if (failureBlock) failureBlock();
     }
     ];
}

- (void) login: (void (^)())successBlock onFailure: (void (^)(int statusCode))failureBlock
{
    [USER_MANAGER
     loginWithStoredCredentials:^()
     {
         NSLog(@"Login success callback");
         if (successBlock) successBlock();
     }
     onFailure:^(int statusCode)
     {
         NSLog(@"Login failure callback:%i", statusCode);
         [Flurry
          logError:@"HTTP.Login"
          message:[NSString stringWithFormat:@"%i", statusCode]
          error:nil
          ];
         if (failureBlock) failureBlock(statusCode);
     }
     ];
}

- (void) syncWithServer: (void (^)())successBlock onFailure: (void (^)())failureBlock
{
    int lastTransId = [[NSUserDefaults standardUserDefaults] integerForKey:PREF_LAST_TRANS_ID];
    
    [TMLocation
     syncWithServer:lastTransId
     onSuccess:^(int transId)
     {
         [TMLocation saveLastTransIdIfHigher:transId];
         
         [TMMeasurement
          syncWithServer:lastTransId
          onSuccess:^(int transId)
          {
              [TMMeasurement saveLastTransIdIfHigher:transId];
              [TMMeasurement associateWithLocations];
              if (successBlock) successBlock();
          }
          onFailure:failureBlock];
     }
     onFailure:failureBlock];
    
    //TODO: sync in parallel?
}

- (void) logout: (void (^)())completionBlock
{
    NSLog(@"Logging out...");
    [Flurry logEvent:@"User.Logout"];
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        
        [USER_MANAGER logout];
        [DATA_MANAGER deleteAllData];
        [[NSUserDefaults standardUserDefaults] setObject:nil forKey:PREF_LAST_TRANS_ID];
        
        dispatch_async(dispatch_get_main_queue(), ^{
            if (completionBlock) completionBlock();
        });
    });
}

@end

@implementation TMServerOpsFactory

static id<TMServerOps> instance;

+ (id<TMServerOps>) getInstance
{
    if (instance == nil)
    {
        instance = [[TMServerOpsImpl alloc] init];
    }
    
    return instance;
}

//for testing. you can set this factory to return a mock object.
+ (void) setInstance: (id<TMServerOps>)mockObject
{
    instance = mockObject;
}

@end

