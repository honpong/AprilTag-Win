//
//  LicenseManager.m
//  RC3DKSampleApp
//
//  Created by Brian on 9/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "LicenseHelper.h"

@implementation LicenseHelper

+ (void) showLicenseValidationError:(NSError *)error
{
    NSString * message = [NSString stringWithFormat:@"There was a problem validating your license. The license error code is: %ld.", (long)error.code];
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"3DK License Error"
                                                    message:message
                                                   delegate:self
                                          cancelButtonTitle:@"OK"
                                          otherButtonTitles:nil];
    [alert show];
}

@end
