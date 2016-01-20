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
#import "OCMock.h"

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

- (void) stubValidLicenseResponse
{
    [self stubLicenseResponse:RCLicenseStatusOK];
}

- (void) stubOfflineResponse
{
    stubRequest(@"POST", URL_LICENSING).
    andFailWithError([NSError errorWithDomain:@"HTTPS request failed" code:1 userInfo:nil]);
}

- (void) stubLicenseResponse:(RCLicenseStatus)statusCode
{
    stubRequest(@"POST", URL_LICENSING).
    andReturn(200).
    withHeaders(@{@"Content-Type": @"application/json"}).
    withBody([NSString stringWithFormat:@"{\"license_type\":0,\"license_status\":%i}", statusCode]);
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

- (void) testLicenseValid
{
    [self stubValidLicenseResponse];
    
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    
    [validator validateLicense:VALID_LICENSE_KEY withCompletionBlock:^(int licenseType, int licenseStatus) {
        [responseArrived fulfill];
    } withErrorBlock:^(NSError* error) {
        [responseArrived fulfill];
        XCTFail(@"%@", error.description);
    }];
    
    [self waitForExpectationsWithTimeout:TIMEOUT handler:nil];
}

- (void) testLicenseOverLimit
{
    [self stubLicenseResponse:RCLicenseStatusOverLimit];
    
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    
    [validator validateLicense:VALID_LICENSE_KEY withCompletionBlock:^(int licenseType, int licenseStatus) {
        [responseArrived fulfill];
        XCTFail(@"Completion block should not be called");
    } withErrorBlock:^(NSError* error) {
        [responseArrived fulfill];
        XCTAssertEqual(error.code, RCLicenseErrorOverLimit);
    }];
    
    [self waitForExpectationsWithTimeout:TIMEOUT handler:nil];
}

- (void) testLicenseRateLimited
{
    [self stubLicenseResponse:RCLicenseStatusRateLimited];
    
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    
    [validator validateLicense:VALID_LICENSE_KEY withCompletionBlock:^(int licenseType, int licenseStatus) {
        [responseArrived fulfill];
        XCTFail(@"Completion block should not be called");
    } withErrorBlock:^(NSError* error) {
        [responseArrived fulfill];
        XCTAssertEqual(error.code, RCLicenseErrorRateLimited);
    }];
    
    [self waitForExpectationsWithTimeout:TIMEOUT handler:nil];
}

- (void) testLicenseSuspended
{
    [self stubLicenseResponse:RCLicenseStatusSuspended];
    
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    
    [validator validateLicense:VALID_LICENSE_KEY withCompletionBlock:^(int licenseType, int licenseStatus) {
        [responseArrived fulfill];
        XCTFail(@"Completion block should not be called");
    } withErrorBlock:^(NSError* error) {
        [responseArrived fulfill];
        XCTAssertEqual(error.code, RCLicenseErrorSuspended);
    }];
    
    [self waitForExpectationsWithTimeout:TIMEOUT handler:nil];
}

- (void) testLicenseInvalid
{
    [self stubLicenseResponse:RCLicenseStatusInvalid];
    
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    
    [validator validateLicense:VALID_LICENSE_KEY withCompletionBlock:^(int licenseType, int licenseStatus) {
        [responseArrived fulfill];
        XCTFail(@"Completion block should not be called");
    } withErrorBlock:^(NSError* error) {
        [responseArrived fulfill];
        XCTAssertEqual(error.code, RCLicenseErrorInvalidKey);
    }];
    
    [self waitForExpectationsWithTimeout:TIMEOUT handler:nil];
}

- (void) testLaxOffline
{
    [self stubOfflineResponse];
    
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    validator.licenseRule = RCLicenseRuleLax;
    
    [validator validateLicense:VALID_LICENSE_KEY withCompletionBlock:^(int licenseType, int licenseStatus) {
        [responseArrived fulfill];
    } withErrorBlock:^(NSError* error) {
        [responseArrived fulfill];
        XCTFail(@"%@", error.description);
    }];
    
    [self waitForExpectationsWithTimeout:TIMEOUT handler:nil];
}

- (void) testLaxOfflineWithPreviousFailure
{
    [self stubOfflineResponse];
    
    id userDefaults = OCMStrictClassMock([NSUserDefaults class]);
    [[[userDefaults stub] andReturnValue:@YES] boolForKey:PREF_LICENSE_INVALID];
    
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:userDefaults];
    validator.licenseRule = RCLicenseRuleLax;
    
    [validator validateLicense:VALID_LICENSE_KEY withCompletionBlock:^(int licenseType, int licenseStatus) {
        [responseArrived fulfill];
        XCTFail(@"Completion block should not be called");
    } withErrorBlock:^(NSError* error) {
        [responseArrived fulfill];
        XCTAssertEqual(error.code, RCLicenseErrorHttpFailure);
    }];
    
    [self waitForExpectationsWithTimeout:TIMEOUT handler:nil];
}

- (void) testLaxOnlineWithPreviousFailure
{
    [self stubValidLicenseResponse];
    
    id userDefaults = OCMStrictClassMock([NSUserDefaults class]);
    [[[userDefaults stub] andReturnValue:@YES] boolForKey:PREF_LICENSE_INVALID];
    [[userDefaults stub] setBool:NO forKey:PREF_LICENSE_INVALID];
    
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:userDefaults];
    validator.licenseRule = RCLicenseRuleLax;
    
    [validator validateLicense:VALID_LICENSE_KEY withCompletionBlock:^(int licenseType, int licenseStatus) {
        [responseArrived fulfill];
    } withErrorBlock:^(NSError* error) {
        [responseArrived fulfill];
        XCTFail(@"%@", error.description);
    }];
    
    [self waitForExpectationsWithTimeout:TIMEOUT handler:nil];
}

- (void) testViewARBundleId
{
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:@"com.viewar.kareshopguid" withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    validator.licenseRule = RCLicenseRuleBundleID;
    validator.allowBundleID = @"com.viewar.kareshopguid";
    
    [validator validateLicense:VALID_LICENSE_KEY withCompletionBlock:^(int licenseType, int licenseStatus) {
        [responseArrived fulfill];
    } withErrorBlock:^(NSError* error) {
        [responseArrived fulfill];
        XCTFail(@"%@", error.description);
    }];
    
    [self waitForExpectationsWithTimeout:TIMEOUT handler:nil];
}

- (void) testBundleIdDenied
{
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    validator.licenseRule = RCLicenseRuleBundleID;
    validator.allowBundleID = @"com.viewar.kareshopguid";
    
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

- (void) testOffline
{
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    validator.licenseRule = RCLicenseRuleOffline;
    
    [validator validateLicense:@"garbage" withCompletionBlock:^(int licenseType, int licenseStatus) {
        [responseArrived fulfill];
    } withErrorBlock:^(NSError* error) {
        [responseArrived fulfill];
        XCTFail(@"%@", error.description);
    }];
    
    [self waitForExpectationsWithTimeout:TIMEOUT handler:nil];
}

@end
