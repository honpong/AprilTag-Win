//
//  LicenseHelper.h
//  RC3DKSampleApp
//
//  Created by Brian on 9/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

#define API_KEY @"YOUR_API_KEY_HERE"

@interface LicenseHelper : NSObject

+ (void)showLicenseStatusError:(int)licenseStatus;
+ (void)showLicenseValidationError:(NSError *)error;

@end
