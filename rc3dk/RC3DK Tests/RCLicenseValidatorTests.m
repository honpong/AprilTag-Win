//
//  RCLicenseValidatorTests.m
//  RC3DK
//
//  Created by Ben Hirashima on 3/31/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>
#import "RCLicenseValidator.h"
#import "RCPrivateHTTPClient.h"
#import "RCConstants.h"
#import "RCLicenseError.h"
#import "Nocilla.h"

#define TIMEOUT 10.
#define URL_LICENSING @"https://app.realitycap.com/api/v1/licensing/"
#define VALID_LICENSE_KEY @"D3bed93A58f8A25FDF7Cbc4da0634D" // Ben's key
#define BUNDLE_ID @"com.realitycap.tapemeasure"
#define VENDOR_ID @"E621E1F8-C36C-495A-93FC-0C247A3E6E5F" // random example

@interface RCLicenseValidatorTests : XCTestCase

@end

@implementation RCLicenseValidatorTests
{
    BOOL done;
    XCTestExpectation* responseArrived;
}

- (void)setUp
{
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
    [RCPrivateHTTPClient initWithBaseUrl:API_BASE_URL withAcceptHeader:API_HEADER_ACCEPT withApiVersion:API_VERSION];
    [[LSNocilla sharedInstance] start];
    responseArrived = [self expectationWithDescription:@"HTTP response arrived"];
}

- (void)tearDown
{
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [[LSNocilla sharedInstance] stop];
    [[LSNocilla sharedInstance] clearStubs];
    [super tearDown];
}

- (BOOL)waitForCompletion:(NSTimeInterval)timeoutSecs
{
    NSDate *timeoutDate = [NSDate dateWithTimeIntervalSinceNow:timeoutSecs];
    
    do {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:timeoutDate];
        if([timeoutDate timeIntervalSinceNow] < 0.0)
            break;
    } while (!done);
    
    return done;
}

- (void)testLicenseValid
{
    stubRequest(@"POST", URL_LICENSING).
    andReturn(200).
    withHeaders(@{@"Content-Type": @"application/json"}).
    withBody(@"{\"license_type\":0,\"license_status\":0}");
    
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    
    [validator validateLicense:VALID_LICENSE_KEY withCompletionBlock:^(int licenseType, int licenseStatus) {
        [responseArrived fulfill];
    } withErrorBlock:^(NSError* error) {
        [responseArrived fulfill];
        XCTFail(@"%@", error.description);
    }];
    
    [self waitForExpectationsWithTimeout:TIMEOUT handler:nil];
}

- (void) testMissingKey
{
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    
    [validator
     validateLicense:nil
     withCompletionBlock:^(int licenseType, int licenseStatus){
         [responseArrived fulfill];
         XCTFail(@"Completion block should not be called");
     }
     withErrorBlock:^(NSError* error){
         [responseArrived fulfill];
         XCTAssertEqual(error.code, RCLicenseErrorMissingKey, @"Error code should be RCLicenseErrorMissingKey");
     }];
    
    [self waitForExpectationsWithTimeout:TIMEOUT handler:nil];
}

- (void) testMalformedKey
{
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    
    [validator
     validateLicense:@"D3bed93A58f8A25FDF7Cbc4da0634D_too_long"
     withCompletionBlock:^(int licenseType, int licenseStatus){
         [responseArrived fulfill];
         XCTFail(@"Completion block should not be called");
     }
     withErrorBlock:^(NSError* error){
         [responseArrived fulfill];
         XCTAssertEqual(error.code, RCLicenseErrorMalformedKey, @"Error code should be RCLicenseErrorMalformedKey");
     }];
    
    [self waitForExpectationsWithTimeout:TIMEOUT handler:nil];
}

- (void) testMissingBundleId
{
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:nil withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    
    [validator
     validateLicense:VALID_LICENSE_KEY
     withCompletionBlock:^(int licenseType, int licenseStatus){
         [responseArrived fulfill];
         XCTFail(@"Completion block should not be called");
     }
     withErrorBlock:^(NSError* error){
         [responseArrived fulfill];
         XCTAssertEqual(error.code, RCLicenseErrorBundleIdMissing, @"Error code should be RCLicenseErrorBundleIdMissing");
     }];
    
    [self waitForExpectationsWithTimeout:TIMEOUT handler:nil];
}

- (void) testMissingVendorId
{
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:nil withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    
    [validator
     validateLicense:VALID_LICENSE_KEY
     withCompletionBlock:^(int licenseType, int licenseStatus){
         [responseArrived fulfill];
         XCTFail(@"Completion block should not be called");
     }
     withErrorBlock:^(NSError* error){
         [responseArrived fulfill];
         XCTAssertEqual(error.code, RCLicenseErrorVendorIdMissing, @"Error code should be RCLicenseErrorVendorIdMissing");
     }];
    
    [self waitForExpectationsWithTimeout:TIMEOUT handler:nil];
}



@end
