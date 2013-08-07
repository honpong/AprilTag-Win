//
//  RCHasher.m
//  RCCore
//
//  Created by Ben Hirashima on 3/6/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCHasher.h"

@implementation RCHasher

+ (NSString*)computeSHA256DigestForString:(NSString*)input
{
    
    const char *cstr = [input cStringUsingEncoding:NSUTF8StringEncoding];
    NSData *data = [NSData dataWithBytes:cstr length:input.length];
    uint8_t digest[CC_SHA256_DIGEST_LENGTH];
    
    // This is an iOS5-specific method.
    // It takes in the data, how much data, and then output format, which in this case is an int array.
    CC_SHA256(data.bytes, data.length, digest);
    
    // Setup our Objective-C output.
    NSMutableString* output = [NSMutableString stringWithCapacity:CC_SHA256_DIGEST_LENGTH * 2];
    
    // Parse through the CC_SHA256 results (stored inside of digest[]).
    for(int i = 0; i < CC_SHA256_DIGEST_LENGTH; i++) {
        [output appendFormat:@"%02x", digest[i]];
    }
    
    return output;
}

+ (NSString*) getSaltedAndHashedString:(NSUInteger)hash
{
    NSString *salt = @"cDxGL&jveoh73Qtz^vK872MCPfp8qlsY";
    return [RCHasher computeSHA256DigestForString:[NSString stringWithFormat:@"%@,%i", salt, hash]];
}

@end
