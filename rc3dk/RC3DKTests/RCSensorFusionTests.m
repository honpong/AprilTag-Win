//
//  RCSensorFusionTests.m
//  RC3DK
//
//  Created by Ben Hirashima on 8/26/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <SenTestingKit/SenTestingKit.h>
#import "RCConstants.h"
#import "RCPrivateHTTPClient.h"
#import "RCSensorFusion.h"

@interface RCSensorFusionTests : SenTestCase

@end

@implementation RCSensorFusionTests
{
    BOOL done;
}

+ (void)setUp
{
    [RCPrivateHTTPClient initWithBaseUrl:API_BASE_URL withAcceptHeader:API_HEADER_ACCEPT withApiVersion:API_VERSION];
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

- (void)setUp
{
    [super setUp];
    done = NO;
}

- (void)tearDown
{
    // Put teardown code here; it will be run once, after the last test case.
    [super tearDown];
}

// see comment in implementation of validateLicense. bundle id must be hard coded to run this test.
- (void)testLicenseValidationWithLiveServer
{
    RCSensorFusion* sensorFusion = [RCSensorFusion sharedInstance];
    
    [sensorFusion
     validateLicense:@"d3a29900eb99b63af0310b83e58bd52a"
     withCompletionBlock:^(int licenseType, int licenseStatus){
         DLog(@"License type: %i, status: %i", licenseType, licenseStatus);
         done = YES;
     }
     withErrorBlock:^(NSError* error){
         STFail(@"%@", error.description);
         done = YES;
     }];
    
    STAssertTrue([self waitForCompletion:10.0], @"Request timed out");
}

- (void)testLicenseValidationFailsWithoutApiKey
{
    RCSensorFusion* sensorFusion = [RCSensorFusion sharedInstance];
    BOOL __block wasErrorBlockCalled = NO;
    
    [sensorFusion
     validateLicense:nil
     withCompletionBlock:^(int licenseType, int licenseStatus){
         STFail(@"Completion block should not be called");
         done = YES;
     }
     withErrorBlock:^(NSError* error){
         STAssertEquals(error.code, RCLicenseErrorApiKeyMissing, @"Error code should be RCLicenseErrorApiKeyMissing = 1");
         wasErrorBlockCalled = YES;
         done = YES;
     }];
    
    STAssertTrue([self waitForCompletion:10.0], @"Request timed out");
    STAssertTrue(wasErrorBlockCalled, @"Error block wasn't called");
}

@end

