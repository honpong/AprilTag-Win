//
//  RCHasherTests.m
//  RCCore
//
//  Created by Ben Hirashima on 3/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCHasherTests.h"
#import "RCHasher.h"

@implementation RCHasherTests

- (void) testComputeSHA256DigestForString
{
    NSString *testString = @"password";
    NSString *hash1 = [RCHasher computeSHA256DigestForString:testString];
    NSString *hash2 = [RCHasher computeSHA256DigestForString:testString];
    
    STAssertNotNil(hash1, @"Returned nil hash");
    STAssertTrue(hash1.length > 0, @"Returned zero length hash");
    STAssertTrue([hash1 isEqualToString:hash2], @"Two hashes of the same string are not equal");
}

- (void) testGetSaltedAndHashedString
{
    NSString *testString = @"password";
    NSString *hash1 = [RCHasher computeSHA256DigestForString:testString];
    NSString *hash2 = [RCHasher computeSHA256DigestForString:testString];
    
    STAssertNotNil(hash1, @"Returned nil hash");
    STAssertTrue(hash1.length > 0, @"Returned zero length hash");
    STAssertTrue([hash1 isEqualToString:hash2], @"Two hashes of the same string are not equal");
}

@end
