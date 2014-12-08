//
//  NSString+RCString.m
//  RCCore
//
//  Created by Ben Hirashima on 2/26/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "NSString+RCString.h"

@implementation NSString (RCString)

- (BOOL)containsString:(NSString*)search
{
    if (search == nil || self.length == 0) return NO;
    return [self rangeOfString:search].location != NSNotFound;
}

- (BOOL) beginsWithString:(NSString*)search
{
    if (search == nil || self.length == 0 || search.length > self.length) return NO;
    return [[self substringToIndex:search.length] isEqualToString:search];
}

- (BOOL) endsWithString:(NSString*)search
{
    if (search == nil || self.length == 0 || search.length > self.length) return NO;
    return [[self substringFromIndex:self.length - search.length] isEqualToString:search];
}
@end
