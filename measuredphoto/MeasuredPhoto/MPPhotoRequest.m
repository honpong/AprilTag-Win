//
//  MPPhotoRequest.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 10/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPPhotoRequest.h"
#import <TrueMeasureSDK/TrueMeasureSDK.h>
#import <RC3DK/RC3DK.h>

static MPPhotoRequest *instance = nil;

@interface MPPhotoRequest ()

@property (nonatomic, readwrite) BOOL isRepliedTo;

@end

@implementation MPPhotoRequest
{
    BOOL isLicenseValid;
}
@synthesize url, apiKey, apiVersion, action, sourceApp, isRepliedTo, dateReceived;

+ (MPPhotoRequest*) lastRequest
{
    return instance;
}

+ (void) setLastRequest:(NSURL*)url withSourceApp:(NSString*)bundleId
{
    instance = [[MPPhotoRequest alloc] initWithUrl:url withSourceApp:bundleId];
}

- (id) initWithUrl:(NSURL *)url_ withSourceApp:(NSString *)bundleId
{
    if (self = [super init])
    {
        dateReceived = [NSDate date];
        isRepliedTo = NO;
        sourceApp = bundleId;
        
        // check "host", which we define as the action the url is requesting
        url = url_;
        action = url.host;
        if (![action isEqual: @"measuredphoto"])
        {
            [self returnErrorToCallingApp:RCTrueMeasureErrorCodeInvalidAction];
            return self;
        }
        
        // check the query string
        NSArray* pairs = [url.query componentsSeparatedByString:@"&"];
        if (pairs.count == 0)
        {
            [self returnErrorToCallingApp:RCTrueMeasureErrorCodeMissingApiKey];
            return self;
        }
        
        NSMutableDictionary* params = [[NSMutableDictionary alloc] initWithCapacity:pairs.count];
        for (NSString* pair in pairs)
        {
            NSArray* keyAndValue = [pair componentsSeparatedByString:@"="];
            if (keyAndValue.count != 2) continue;
            [params setObject:keyAndValue[1] forKey:keyAndValue[0]];
        }
        if (params.count == 0)
        {
            [self returnErrorToCallingApp:RCTrueMeasureErrorCodeMissingApiKey];
            return self;
        }
        
        apiKey = [params objectForKey:kRCQueryStringApiKey];
        if (apiKey == nil || apiKey.length == 0)
        {
            [self returnErrorToCallingApp:RCTrueMeasureErrorCodeMissingApiKey];
            return self;
        }
        
        isLicenseValid = NO;
        __weak MPPhotoRequest* weakSelf = self;
        [SENSOR_FUSION
         validateLicense:apiKey
         withCompletionBlock:^(int licenseType, int licenseStatus) {
             if (licenseStatus == RCLicenseStatusOK)
             {
                 if (licenseType == RCLicenseTypeEvalutaion) // TODO truemeasure license type
                 {
                     isLicenseValid = YES;
                 }
                 else
                 {
                     [weakSelf returnErrorToCallingApp:RCTrueMeasureErrorCodeWrongLicenseType];
                 }
             }
             else
             {
                 [weakSelf returnErrorToCallingApp:RCTrueMeasureErrorCodeLicenseInvalid];
             }
         }
         withErrorBlock:^(NSError *error) {
             DLog(@"License validation failure: %@", error);
             [weakSelf returnErrorToCallingApp:RCTrueMeasureErrorCodeLicenseValidationFailure];
         }];
    }
    
    return self;
}

- (void) returnErrorToCallingApp:(RCTrueMeasureErrorCode)code
{
    NSString* urlString = [NSString stringWithFormat:@"%@.truemeasure.v1://error?code=%i", sourceApp, code];
    NSURL *myURL = [NSURL URLWithString:urlString];
    if ([[UIApplication sharedApplication] canOpenURL:myURL]) [[UIApplication sharedApplication] openURL:myURL];
    isRepliedTo = YES;
}

@end
