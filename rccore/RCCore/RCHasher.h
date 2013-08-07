//
//  RCHasher.h
//  RCCore
//
//  Created by Ben Hirashima on 3/6/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Security/Security.h>
#import <CommonCrypto/CommonHMAC.h>

@interface RCHasher : NSObject

+ (NSString*)computeSHA256DigestForString:(NSString*)input;
+ (NSString*)getSaltedAndHashedString:(NSUInteger)hash;

@end
