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
         [TMAnalytics logEvent:@"User.AnonAccountCreated"];
         if (successBlock) successBlock();
     }
     onFailure:^(int statusCode)
     {
         DLog(@"createAnonAccount failure callback:%i", statusCode);
         [TMAnalytics
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
         DLog(@"Login success callback");
         if (successBlock) successBlock();
     }
     onFailure:^(int statusCode)
     {
         DLog(@"Login failure callback:%i", statusCode);
         [TMAnalytics
          logError:@"HTTP.Login"
          message:[NSString stringWithFormat:@"%i", statusCode]
          error:nil
          ];
         if (failureBlock) failureBlock(statusCode);
     }
     ];
}

- (void) syncWithServer: (void (^)(BOOL updated))successBlock onFailure: (void (^)())failureBlock
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
              BOOL updated = transId > lastTransId; // note that this only indicates if measurements were updated, not locations
              if (successBlock) successBlock(updated);
          }
          onFailure:failureBlock];
     }
     onFailure:failureBlock];
    
    //TODO: sync in parallel?
}

- (void) logout: (void (^)())completionBlock
{
    LOGME
    [TMAnalytics logEvent:@"User.Logout"];
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        
        [USER_MANAGER logout];
        [DATA_MANAGER deleteAllData];
        [[NSUserDefaults standardUserDefaults] setObject:nil forKey:PREF_LAST_TRANS_ID];
        
        dispatch_async(dispatch_get_main_queue(), ^{
            if (completionBlock) completionBlock();
        });
    });
}


- (void) postJsonData:(NSDictionary*)params onSuccess:(void (^)())successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    DLog(@"POST %@\n%@", API_DATUM_LOGGED, params);
        
    [HTTP_CLIENT
     postPath:API_DATUM_LOGGED
     parameters:params
     success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         DLog(@"POST Response\n%@", operation.responseString);
         if (successBlock) successBlock();
     }
     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {         
         DLog(@"Failed to POST object: %i %@", operation.response.statusCode, operation.responseString);
         [TMAnalytics
          logError:@"HTTP.POST"
          message:[NSString stringWithFormat:@"%i: %@", operation.response.statusCode, operation.request.URL.relativeString]
          error:error
          ];
         
         NSString *requestBody = [[NSString alloc] initWithData:operation.request.HTTPBody encoding:NSUTF8StringEncoding];
         DLog(@"Failed request body:\n%@", requestBody);
         if (failureBlock) failureBlock(operation.response.statusCode);
     }
     ];
}

- (void) postDeviceCalibration:(void (^)())successBlock onFailure:(void (^)(int statusCode))failureBlock
{
    LOGME;
    /*
    NSDictionary* calibrationData = [RCCalibration getCalibrationAsDictionary];
    if (calibrationData == nil)
    {
        DLog(@"Calibration data is nill");
        failureBlock(0);
    }
    */
    NSString *jsonString = [RCCalibration getCalibrationAsString];
    /*
    NSError *error;
    NSData *jsonData = [NSJSONSerialization dataWithJSONObject:jsonDict options:0 error:&error];
    NSString *jsonString = [[NSString alloc] initWithData:jsonData encoding:NSUTF8StringEncoding];

    if (error)
    {
        DLog(@"JSON serialization error: %@", error);
        failureBlock(0);
    }
    */
    NSDictionary* postParams = @{ JSON_KEY_FLAG:[NSNumber numberWithInt: JsonBlobFlagCalibrationData], JSON_KEY_BLOB: jsonString };
    
    [self
     postJsonData:postParams
     onSuccess:^()
     {
         if (successBlock) successBlock();
     }
     onFailure:^(int statusCode)
     {
         if (failureBlock) failureBlock(statusCode);
     }
     ];
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

