//
//  LicenseManager.m
//  RC3DKSampleApp
//
//  Created by Brian on 9/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <RC3DK/RC3DK.h>

#import "LicenseHelper.h"

@implementation LicenseHelper

+ (void)showLicenseStatusError:(int)licenseStatus
{
    NSString * message = [NSString stringWithFormat:@"There is a problem with your license. Please make sure you've correctly entered your API key in LicenseHelper.h. The license status code is: %i.", licenseStatus];
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"RC3DK License Status Error"
                                                    message:message
                                                   delegate:self
                                          cancelButtonTitle:@"OK"
                                          otherButtonTitles:nil];
    [alert show];
}

+ (void)showLicenseValidationError:(NSError *)error
{
    NSString * message = [NSString stringWithFormat:@"There was a problem validating your license. The license error code is: %i.", error.code];
    UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"RC3DK License Error"
                                                    message:message
                                                   delegate:self
                                          cancelButtonTitle:@"OK"
                                          otherButtonTitles:nil];
    [alert show];
}

@end
