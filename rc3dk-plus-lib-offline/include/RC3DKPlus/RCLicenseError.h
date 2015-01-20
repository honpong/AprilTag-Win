//
//  RCLicenseError.h
//  RC3DK
//
//  Created by Ben Hirashima on 6/17/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

/**
 Represents the type of license validation error.
 */
typedef NS_ENUM(int, RCLicenseErrorCode)
{
    /** An unknown error occured. Please contact support. */
    RCLicenseErrorUnknown = 1,
    /** The license key provided was nil or zero length */
    RCLicenseErrorMissingKey = 2,
    /** The license key is the wrong length. Must be exactly 30 characters. */
    RCLicenseErrorMalformedKey = 3,
    /** The license key is invalid. Please obtain a license key from RealityCap. */
    RCLicenseErrorInvalidKey = 4,
    /** We weren't able to get the app's bundle ID from the system */
    RCLicenseErrorBundleIdMissing = 5,
    /** We weren't able to get the identifier for vendor from the system */
    RCLicenseErrorVendorIdMissing = 6,
    /** The license server returned an empty response */
    RCLicenseErrorEmptyResponse = 7,
    /** Failed to deserialize the response from the license server */
    RCLicenseErrorDeserialization = 8,
    /** The license server returned invalid data */
    RCLicenseErrorInvalidResponse = 9,
    /** Failed to execute the HTTP request. See underlying error for details. */
    RCLicenseErrorHttpFailure = 10,
    /** We got an HTTP error status from the license server. */
    RCLicenseErrorHttpError = 11,
    /** The maximum number of sensor fusion sessions has been reached for the current time period. Contact customer service if you wish to change your license type. */
    RCLicenseErrorOverLimit = 12,
    /** API use has been rate limited. Try again after a short time. */
    RCLicenseErrorRateLimited = 13,
    /** Account suspended. Please contact support. */
    RCLicenseErrorSuspended = 14
};

@interface RCLicenseError : NSError

/**
 Contains a value corresponding to a RCLicenseErrorCode
 */
- (NSInteger) code;

@end
