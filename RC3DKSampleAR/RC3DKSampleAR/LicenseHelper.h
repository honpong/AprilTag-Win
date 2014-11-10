//
//  LicenseHelper.h
//  RC3DKSampleApp
//
//  Created by Brian on 9/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

#error You must insert your 3DK license key here and delete this line
#define SDK_LICENSE_KEY @"YOUR_LICENSE_KEY_HERE"

@interface LicenseHelper : NSObject

+ (void)showLicenseValidationError:(NSError *)error;

@end
