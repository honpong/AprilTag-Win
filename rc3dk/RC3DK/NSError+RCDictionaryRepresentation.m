//
//  NSError+RCDictionaryRepresentation.m
//  RC3DK
//
//  Created by Ben Hirashima on 2/20/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#import "NSError+RCDictionaryRepresentation.h"

@implementation NSError (RCDictionaryRepresentation)

- (NSDictionary*) dictionaryRepresentation
{
    NSDictionary* dict = @{
                           @"class": [self.class description],
                           @"code": @(self.code),
                           @"description": self.description
                           };
    return dict;
}

@end
