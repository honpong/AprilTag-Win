//
//  RCLicenseError.m
//  RC3DK
//
//  Created by Ben Hirashima on 6/17/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "RCLicenseError.h"

@implementation RCLicenseError

- (NSInteger) code
{
    return [super code];
}

- (NSDictionary*) dictionaryRepresentation
{
    NSDictionary* dict = @{
                           @"class": [self.class description],
                           @"code": @(self.code)
                           };
    return dict;
}

@end