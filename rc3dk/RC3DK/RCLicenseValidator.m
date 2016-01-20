//
//  RCLicenseValidator.m
//  RC3DK
//
//  Created by Ben Hirashima on 3/31/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#import "RCLicenseValidator.h"
#import "RCConstants.h"
#import "RCSensorFusion.h"

@implementation RCLicenseValidator
{
    NSString* bundleId;
    RCAFHTTPClient* httpClient;
    NSUserDefaults* userDefaults;
    NSString* vendorId;
}
@synthesize licenseRule, allowBundleID;

+ (RCLicenseValidator*) initWithBundleId:(NSString*)bundleId withVendorId:(NSString*)vendorId withHTTPClient:(RCAFHTTPClient*)httpClient withUserDefaults:(NSUserDefaults*)userDefaults
{
    return [[RCLicenseValidator alloc] initWithBundleId:bundleId withVendorId:vendorId withHTTPClient:httpClient withUserDefaults:userDefaults];
}

- (instancetype)initWithBundleId:(NSString*)bundleId_ withVendorId:(NSString*)vendorId_ withHTTPClient:(RCAFHTTPClient*)httpClient_ withUserDefaults:(NSUserDefaults*)userDefaults_
{
    self = [super init];
    if (!self) return nil;
    
    bundleId = bundleId_;
    httpClient = httpClient_;
    userDefaults = userDefaults_;
    vendorId = vendorId_;
    licenseRule = RCLicenseRuleStrict;
    
    return self;
}

- (void) validateLicense:(NSString*)apiKey withCompletionBlock:(void (^)(int licenseType, int licenseStatus))completionBlock withErrorBlock:(void (^)(NSError*))errorBlock
{
    if (self.licenseRule == RCLicenseRuleOffline)
    {
        DLog(@"Skipping license check");
        if (completionBlock) completionBlock(RCLicenseTypeFull, RCLicenseStatusOK);
        return;
    }
    
    if (self.licenseRule == RCLicenseRuleBundleID && self.allowBundleID && self.allowBundleID.length > 0)
    {
        if ([bundleId isEqualToString:self.allowBundleID])
        {
            if (completionBlock) completionBlock(RCLicenseTypeFull, RCLicenseStatusOK);
        }
        else
        {
            if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorBundleIdMissing userInfo:@{NSLocalizedDescriptionKey: @"Failed to validate license. Wrong bundle ID.", NSLocalizedFailureReasonErrorKey: @"Wrong bundle ID."}]);
        }
        return;
    }
    
    // everything below here should get optimized out by the compiler if we're skipping the license check
    
    if (apiKey == nil || apiKey.length == 0)
    {
        if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorMissingKey userInfo:@{NSLocalizedDescriptionKey: @"Failed to validate license. License key was nil or zero length.", NSLocalizedFailureReasonErrorKey: @"License key was nil or zero length."}]);
        return;
    }
    
    if ([apiKey length] != 30)
    {
        if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorMalformedKey userInfo:@{NSLocalizedDescriptionKey: @"Failed to validate license. License key was malformed. It must be exactly 30 characters in length.", NSLocalizedFailureReasonErrorKey: @"License key must be 30 characters in length."}]);
        return;
    }
    
    if (bundleId == nil || bundleId.length == 0)
    {
        if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorBundleIdMissing userInfo:@{NSLocalizedDescriptionKey: @"Failed to validate license. Could not get bundle ID.", NSLocalizedFailureReasonErrorKey: @"Could not get bundle ID."}]);
        return;
    }
    
    if (vendorId == nil || vendorId.length == 0)
    {
        if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorVendorIdMissing userInfo:@{NSLocalizedDescriptionKey: @"Failed to validate license. Could not get ID for vendor.", NSLocalizedFailureReasonErrorKey: @"Could not get ID for vendor."}]);
        return;
    }
    
    NSDictionary *params = @{@"requested_resource": @"3DK_TR",
                             @"api_key": apiKey,
                             @"bundle_id": bundleId,
                             @"vendor_id": vendorId};
    
    [httpClient
     postPath:API_LICENSING_POST
     parameters:params
     success:^(RCAFHTTPRequestOperation *operation, id JSON)
     {
         DLog(@"License completion %li\n%@", (long)operation.response.statusCode, operation.responseString);
         if (operation.response.statusCode != 200)
         {
             if (self.licenseRule == RCLicenseRuleLax) completionBlock(-1, RCLicenseStatusOK);
             else if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorHttpError userInfo:@{NSLocalizedDescriptionKey: [NSString stringWithFormat:@"Failed to validate license. HTTP response code %li.", (long)operation.response.statusCode], NSLocalizedFailureReasonErrorKey: [NSString stringWithFormat:@"HTTP status %li: %@", (long)operation.response.statusCode, operation.responseString]}]);
             return;
         }
         
         if (JSON == nil)
         {
             if (self.licenseRule == RCLicenseRuleLax) completionBlock(-1, RCLicenseStatusOK);
             else if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorEmptyResponse userInfo:@{NSLocalizedDescriptionKey: @"Failed to validate license. Response body was empty.", NSLocalizedFailureReasonErrorKey: @"Response body was empty."}]);
             return;
         }
         
         NSError* serializationError;
         NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:0 error:&serializationError];
         if (serializationError || response == nil)
         {
             if (self.licenseRule == RCLicenseRuleLax) completionBlock(-1, RCLicenseStatusOK);
             else if (errorBlock)
             {
                 NSMutableDictionary* userInfo = [NSMutableDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. Failed to deserialize response.", NSLocalizedDescriptionKey, @"Failed to deserialize response.", NSLocalizedFailureReasonErrorKey, nil];
                 if (serializationError) userInfo[NSUnderlyingErrorKey] = serializationError;
                 errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorDeserialization userInfo:userInfo]);
             }
             return;
         }
         
         NSNumber* licenseStatusString = response[@"license_status"];
         NSNumber* licenseTypeString = response[@"license_type"];
         
         if (licenseStatusString == nil || licenseTypeString == nil)
         {
             if (self.licenseRule == RCLicenseRuleLax) completionBlock(-1, RCLicenseStatusOK);
             else if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorInvalidResponse userInfo:@{NSLocalizedDescriptionKey: @"Failed to validate license. Invalid response from server.", NSLocalizedFailureReasonErrorKey: @"Invalid response from server."}]);
             return;
         }
         
         int licenseStatus = [licenseStatusString intValue];
         int licenseType = [licenseTypeString intValue];
         
         switch (licenseStatus)
         {
             case RCLicenseStatusOK:
                 [userDefaults setBool:NO forKey:PREF_LICENSE_INVALID];
                 if (completionBlock) completionBlock(licenseType, licenseStatus);
                 break;
                 
             case RCLicenseStatusOverLimit:
                 [userDefaults setBool:YES forKey:PREF_LICENSE_INVALID];
                 if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorOverLimit userInfo:nil]);
                 break;
                 
             case RCLicenseStatusRateLimited:
                 [userDefaults setBool:YES forKey:PREF_LICENSE_INVALID];
                 if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorRateLimited userInfo:nil]);
                 break;
                 
             case RCLicenseStatusSuspended:
                 [userDefaults setBool:YES forKey:PREF_LICENSE_INVALID];
                 if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorSuspended userInfo:nil]);
                 break;
                 
             case RCLicenseStatusInvalid:
                 [userDefaults setBool:YES forKey:PREF_LICENSE_INVALID];
                 if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorInvalidKey userInfo:nil]);
                 break;
                 
             default:
                 if (self.licenseRule == RCLicenseRuleLax) completionBlock(-1, RCLicenseStatusOK);
                 else if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorUnknown userInfo:nil]);
                 break;
         }
     }
     failure:^(RCAFHTTPRequestOperation *operation, NSError *error)
     {
         DLog(@"License failure: %li\n%@", (long)operation.response.statusCode, operation.responseString);
         
         if (self.licenseRule == RCLicenseRuleLax && ![userDefaults boolForKey:PREF_LICENSE_INVALID])
         {
             if (completionBlock) completionBlock(-1, RCLicenseStatusOK);
             return;
         }
         
         if (errorBlock)
         {
             NSMutableDictionary* userInfo = [NSMutableDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. HTTPS request failed.", NSLocalizedDescriptionKey, @"HTTPS request failed. See underlying error.", NSLocalizedFailureReasonErrorKey, nil];
             if (error) userInfo[NSUnderlyingErrorKey] = error;
             errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorHttpFailure userInfo:userInfo]);
         }
     }
     ];
}

@end
