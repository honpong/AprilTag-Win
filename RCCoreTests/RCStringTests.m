//
//  RCStringTests.m
//  RCCore
//
//  Created by Ben Hirashima on 3/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCStringTests.h"
#import "NSString+RCString.h"

@implementation RCStringTests

- (void) testContainsString
{
    NSString *testString = @"my dog has fleas";
    
    STAssertTrue([testString containsString:@"my"], @"Failed to find substring at beginning of string");
    STAssertTrue([testString containsString:@"fleas"], @"Failed to find substring at end of string");
    STAssertTrue([testString containsString:@"dog"], @"Failed to find substring in middle of string");
    STAssertTrue([testString containsString:@"dog has"], @"Failed to find two word substring");
    STAssertTrue([testString containsString:@"fle"], @"Failed to find partial word substring");
    
    STAssertFalse([testString containsString:@"cat"], @"Found string that isn't there");
}

@end
