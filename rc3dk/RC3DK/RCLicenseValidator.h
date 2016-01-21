//
//  RCLicenseValidator.h
//  RC3DK
//
//  Created by Ben Hirashima on 3/31/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "RCPrivateHTTPClient.h"

#define PREF_LICENSE_INVALID @"&C55YqEFpDH5" // nonsense, so that it's meaning is obfuscated

typedef NS_ENUM(int, RCLicenseType)
{
    /** This license provide full access with a limited number of uses per month. */
    RCLicenseTypeEvalutaion = 0,
    /** This license provides access to 6DOF device motion data only. Obsolete. */
    RCLicenseTypeMotionOnly = 16,
    /** This license provides full access to 6DOF device motion and point cloud data. */
    RCLicenseTypeFull = 32
};

typedef NS_ENUM(int, RCLicenseStatus)
{
    /** Authorized. You may proceed. */
    RCLicenseStatusOK = 0,
    /** The maximum number of sensor fusion sessions has been reached for the current time period. Contact customer service if you wish to change your license type. */
    RCLicenseStatusOverLimit = 1,
    /** API use has been rate limited. Try again after a short time. */
    RCLicenseStatusRateLimited = 2,
    /** Account suspended. Please contact customer service. */
    RCLicenseStatusSuspended = 3,
    /** License key not found */
    RCLicenseStatusInvalid = 4
};

typedef NS_ENUM(int, RCLicenseRule)
{
    /** Checks license server every run. */
    RCLicenseRuleStrict = 0,
    /** Allows use if server can't be reached, unless we've received a denial in the past. */
    RCLicenseRuleLax= 10,
    /** Allows offline use of a specific bundle ID */
    RCLicenseRuleBundleID = 20,
};

@interface RCLicenseValidator : NSObject

@property (nonatomic) RCLicenseRule licenseRule;
@property (nonatomic) NSString* allowBundleID; // if licenseRule == RCLicenseRuleBundleID, allows offline use with this bundle ID only

+ (RCLicenseValidator*) initWithBundleId:(NSString*)bundleId withVendorId:(NSString*)vendorId withHTTPClient:(RCAFHTTPClient*)httpClient withUserDefaults:(NSUserDefaults*)userDefaults;

- (instancetype) initWithBundleId:(NSString*)bundleId withVendorId:(NSString*)vendorId withHTTPClient:(RCAFHTTPClient*)httpClient withUserDefaults:(NSUserDefaults*)userDefaults;
- (void) validateLicense:(NSString*)apiKey withCompletionBlock:(void (^)(int licenseType, int licenseStatus))completionBlock withErrorBlock:(void (^)(NSError*))errorBlock;

@end
