//
//  TMLiveServerTests.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/8/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMLiveServerTests.h"
#import "TMSyncable+TMSyncableSync.h"
#import "TMMeasurement+TMMeasurementSync.h"
#import "TMLocation+TMLocationSync.h"
#import "RCHttpClient.h"
#import "RCCore/RCUserManagerFactory.h"
#import "TMServerOpsFactory.h"
#import "RCCore/RCCalibration.h"

@implementation TMLiveServerTests

- (void) setUp
{
    NSLog(@"TMLiveServerTests");
    [super setUp];
    done = NO;
    
    [RCHTTPClient initWithBaseUrl:API_BASE_URL withAcceptHeader:API_HEADER_ACCEPT withApiVersion:API_VERSION];
    
    if ([USER_MANAGER hasValidStoredCredentials])
    {
        [self login];
    }
    else
    {
        [SERVER_OPS
         createAnonAccount: ^{
             [self login];
         }
         onFailure: ^{
             STFail(@"Create anon account failed");
         }];       
    }
    
    STAssertTrue([self waitForCompletion:5.0], @"Request timed out");
}

- (void) login
{
    [SERVER_OPS
     login: ^{
         NSLog(@"Login successful");
         done = YES;
     }
     onFailure: ^(int statusCode){
         STFail(@"Login failed with status code %i", statusCode);
     }];
    
    STAssertTrue([self waitForCompletion:5.0], @"Request timed out");
}

- (void) tearDown
{
//    [RCUserManagerFactory setInstance:nil];
    [RCHTTPClient setInstance:nil];
//    [RCUser deleteStoredUser];
    
    [super tearDown];
}

- (BOOL)waitForCompletion:(NSTimeInterval)timeoutSecs {
    NSDate *timeoutDate = [NSDate dateWithTimeIntervalSinceNow:timeoutSecs];
    
    do {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:timeoutDate];
        if([timeoutDate timeIntervalSinceNow] < 0.0)
            break;
    } while (!done);
    
    return done;
}

- (void) testDownloadMeasurements
{
    done = NO;
    
    [TMMeasurement
     downloadChanges:0
     withPage:1
     onSuccess:^(int lastTransId)
     {
         done = YES;
     }
     onFailure:^(int statusCode)
     {
         STFail(@"downloadChanges called failure block with status code %i", statusCode);
         done = YES;
     }
    ];

    STAssertTrue([self waitForCompletion:5.0], @"Request timed out");
}

- (void) testPostJsonData
{
    done = NO;
    
    //make sure some calibration data exists
    corvis_device_parameters params = [RCCalibration getCalibrationData];
    [RCCalibration saveCalibrationData:params];
    
   [[TMServerOpsFactory getInstance]
     postDeviceCalibration:^
     {
         done = YES;
     }
     onFailure:^(int statusCode)
     {
         STFail(@"postJsonData called failure block with status code %i", statusCode);
         done = YES;
     }
     ];
    
    STAssertTrue([self waitForCompletion:5.0], @"Request timed out");
}
@end
