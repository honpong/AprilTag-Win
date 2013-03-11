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
#import "RCCore/RCHttpClientFactory.h"
#import "RCCore/RCUserManagerFactory.h"

@implementation TMLiveServerTests

- (void) setUp
{
    [super setUp];
    done = NO;
    [RCHttpClientFactory initWithBaseUrl:@"https://internal.realitycap.com/" withAcceptHeader:@"application/vnd.realitycap.json; version=1.0" withApiVersion:1];
    
    [USER_MANAGER
     loginWithStoredCredentials:^()
     {
         done = YES;
     }
     onFailure:^(int statusCode)
     {
         STFail(@"loginWithStoredCredentials called failure block with status code %i", statusCode);
         done = YES;
     }];
    
    [self waitForCompletion:5.0];
}

- (void) tearDown
{
    [RCUserManagerFactory setInstance:nil];
    [RCHttpClientFactory setInstance:nil];
    [RCUser deleteStoredUser];
    
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
@end
