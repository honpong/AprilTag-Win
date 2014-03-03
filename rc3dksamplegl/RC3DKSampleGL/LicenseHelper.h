//
//  LicenseHelper.h
//  RC3DKSampleApp
//
//  Created by Brian on 9/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

#define API_KEY @"c6c58b26eD74AB3e726cc8e15E38ab"

@interface LicenseHelper : NSObject

+ (void)showLicenseStatusError:(int)licenseStatus;
+ (void)showLicenseValidationError:(NSError *)error;

@end
