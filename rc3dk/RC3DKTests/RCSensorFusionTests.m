//
//  RCSensorFusionTests.m
//  RC3DK
//
//  Created by Ben Hirashima on 8/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <XCTest/XCTest.h>
#import "RCConstants.h"
#import "RCHttpClient.h"
#import "RCSensorFusion.h"

#define API_VERSION 1
#define API_BASE_URL @"https://internal.realitycap.com/"
#define API_HEADER_ACCEPT @"application/vnd.realitycap.json; version=1.0"
#define API_LICENSING_POST @"api/v" + API_VERSION + "/licensing/"

@interface RCSensorFusionTests : XCTestCase

@end

@implementation RCSensorFusionTests
{
    BOOL done;
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
    [RCHttpClient initWithBaseUrl:API_BASE_URL withAcceptHeader:API_HEADER_ACCEPT withApiVersion:API_VERSION];
}

- (void)tearDown
{
    // Put teardown code here; it will be run once, after the last test case.
    [super tearDown];
}

- (void)testExample
{
    XCTFail(@"No implementation for \"%s\"", __PRETTY_FUNCTION__);
}

- (void)testLicenseValidation
{
    RCSensorFusion* sensorFusion = [RCSensorFusion sharedInstance];
    
    
    
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
    
    XCTest([self waitForCompletion:10.0], @"Request timed out");
}



@end
