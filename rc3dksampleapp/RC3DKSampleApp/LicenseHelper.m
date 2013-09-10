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

+ (void)validateLicenseAndStartProcessingVideo
{
    [[RCSensorFusion sharedInstance] validateLicense:API_KEY withCompletionBlock:^(int licenseType, int licenseStatus){
        if(licenseStatus == RCLicenseStatusOK)
        {
            [[RCSensorFusion sharedInstance] startProcessingVideo];
        }
        else
        {
            NSString * message = [NSString stringWithFormat:@"There is a problem with the status your license. The license status code is: %i", licenseStatus];
            UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"RC3DK License Status Error"
                                                            message:message
                                                           delegate:self
                                                  cancelButtonTitle:@"OK"
                                                  otherButtonTitles:nil];
            [alert show];
        }
    } withErrorBlock:^(NSError * error) {
        NSString * message = [NSString stringWithFormat:@"There was a problem validating your license. The license error code is: %i", error.code];
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"RC3DK License Error"
                                                        message:message
                                                       delegate:self
                                              cancelButtonTitle:@"OK"
                                              otherButtonTitles:nil];
        [alert show];
    }];
}

@end
