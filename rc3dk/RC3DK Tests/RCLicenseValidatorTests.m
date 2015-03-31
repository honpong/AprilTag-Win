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

#define VALID_LICENSE_KEY @"D3bed93A58f8A25FDF7Cbc4da0634D" // Ben's key
#define BUNDLE_ID @"com.realitycap.tapemeasure"
#define VENDOR_ID @"E621E1F8-C36C-495A-93FC-0C247A3E6E5F" // random example

@interface RCLicenseValidatorTests : XCTestCase

@end

@implementation RCLicenseValidatorTests
{
    BOOL done;
}

- (void)setUp
{
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
    [RCPrivateHTTPClient initWithBaseUrl:API_BASE_URL withAcceptHeader:API_HEADER_ACCEPT withApiVersion:API_VERSION];
}

- (void)tearDown
{
    // Put teardown code here. This method is called after the invocation of each test method in the class.
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

- (void)testLiveServer
{
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    
    [validator validateLicense:VALID_LICENSE_KEY withCompletionBlock:^(int licenseType, int licenseStatus) {
        DLog(@"License type: %i, status: %i", licenseType, licenseStatus);
        done = YES;
    } withErrorBlock:^(NSError* error) {
        XCTFail(@"%@", error.description);
        done = YES;
    }];
    
    XCTAssertTrue([self waitForCompletion:10.0], @"Request timed out");
}

- (void) testMissingKey
{
    BOOL __block wasErrorBlockCalled = NO;
    
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    
    [validator
     validateLicense:nil
     withCompletionBlock:^(int licenseType, int licenseStatus){
         XCTFail(@"Completion block should not be called");
         done = YES;
     }
     withErrorBlock:^(NSError* error){
         XCTAssertEqual(error.code, RCLicenseErrorMissingKey, @"Error code should be RCLicenseErrorMissingKey");
         wasErrorBlockCalled = YES;
         done = YES;
     }];
    
    XCTAssertTrue([self waitForCompletion:10.0], @"Request timed out");
    XCTAssertTrue(wasErrorBlockCalled, @"Error block wasn't called");
}

- (void) testMalformedKey
{
    BOOL __block wasErrorBlockCalled = NO;
    
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    
    [validator
     validateLicense:@"D3bed93A58f8A25FDF7Cbc4da0634D_too_long"
     withCompletionBlock:^(int licenseType, int licenseStatus){
         XCTFail(@"Completion block should not be called");
         done = YES;
     }
     withErrorBlock:^(NSError* error){
         XCTAssertEqual(error.code, RCLicenseErrorMalformedKey, @"Error code should be RCLicenseErrorMalformedKey");
         wasErrorBlockCalled = YES;
         done = YES;
     }];
    
    XCTAssertTrue([self waitForCompletion:10.0], @"Request timed out");
    XCTAssertTrue(wasErrorBlockCalled, @"Error block wasn't called");
}

- (void) testMissingBundleId
{
    BOOL __block wasErrorBlockCalled = NO;
    
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:nil withVendorId:VENDOR_ID withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    
    [validator
     validateLicense:VALID_LICENSE_KEY
     withCompletionBlock:^(int licenseType, int licenseStatus){
         XCTFail(@"Completion block should not be called");
         done = YES;
     }
     withErrorBlock:^(NSError* error){
         XCTAssertEqual(error.code, RCLicenseErrorBundleIdMissing, @"Error code should be RCLicenseErrorBundleIdMissing");
         wasErrorBlockCalled = YES;
         done = YES;
     }];
    
    XCTAssertTrue([self waitForCompletion:10.0], @"Request timed out");
    XCTAssertTrue(wasErrorBlockCalled, @"Error block wasn't called");
}

- (void) testMissingVendorId
{
    BOOL __block wasErrorBlockCalled = NO;
    
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:BUNDLE_ID withVendorId:nil withHTTPClient:HTTP_CLIENT withUserDefaults:NSUserDefaults.standardUserDefaults];
    
    [validator
     validateLicense:VALID_LICENSE_KEY
     withCompletionBlock:^(int licenseType, int licenseStatus){
         XCTFail(@"Completion block should not be called");
         done = YES;
     }
     withErrorBlock:^(NSError* error){
         XCTAssertEqual(error.code, RCLicenseErrorVendorIdMissing, @"Error code should be RCLicenseErrorVendorIdMissing");
         wasErrorBlockCalled = YES;
         done = YES;
     }];
    
    XCTAssertTrue([self waitForCompletion:10.0], @"Request timed out");
    XCTAssertTrue(wasErrorBlockCalled, @"Error block wasn't called");
}



@end
