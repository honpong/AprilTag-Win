//
//  LicenseHelper.h
//  RC3DKSampleApp
//
//  Created by Brian on 9/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

#define API_KEY @"D3bed93A58f8A25FDF7Cbc4da0634D"

@interface LicenseHelper : NSObject

+ (void)showLicenseStatusError:(int)licenseStatus;
+ (void)showLicenseValidationError:(NSError *)error;

@end
