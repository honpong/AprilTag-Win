//
//  TMMeasuredPhoto.m
//  TrueMeasureSDK
//
//  Created by Ben Hirashima on 10/15/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMMeasuredPhoto.h"
#import <UIKit/UIKit.h>

#define ERROR_DOMAIN @"com.realitycap.TrueMeasure.ErrorDomain"

NSString* kTMMeasuredPhotoUTI = @"com.realitycap.truemeasure.measuredphoto";

static NSString* const kTMKeyAppVersion = @"kTMKeyAppVersion";
static NSString* const kTMKeyAppBuildNumber = @"kTMKeyAppBuildNumber";
static NSString* const kTMKeyFeaturePoints = @"kTMKeyFeaturePoints";
static NSString* const kTMKeyImageData = @"kTMKeyImageData";

static NSString* const kTMQueryStringPasteboard = @"pasteboard";
static NSString* const kTMQueryStringErrorCode = @"code";

@implementation TMMeasuredPhoto

+ (BOOL) requestMeasuredPhoto:(NSString*)apiKey
{
    return [TMMeasuredPhoto requestMeasuredPhoto:apiKey withApiVersion:kTMApiVersion];
}

+ (BOOL) requestMeasuredPhoto:(NSString*)apiKey withApiVersion:(int)apiVersion
{
    NSURL* myURL = [TMMeasuredPhoto getRequestUrl:apiKey];
    if ([[UIApplication sharedApplication] canOpenURL:myURL])
    {
        [[UIApplication sharedApplication] openURL:myURL];
        return YES;
    }
    else
    {
        return NO;
    }
}

+ (int) getHighestInstalledApiVersion
{
    NSURL* myURL = [TMMeasuredPhoto getRequestUrl:nil];
    return [[UIApplication sharedApplication] canOpenURL:myURL] ? 1 : 0;
}

+ (NSURL*) getRequestUrl:(NSString*)apiKey
{
    NSString* urlString = [NSString stringWithFormat:@"com.realitycap.truemeasure.measuredphoto.v%i://measuredphoto?apikey=%@", kTMApiVersion, apiKey];
    return [NSURL URLWithString:urlString];
}

+ (TMMeasuredPhoto*) retrieveMeasuredPhotoWithUrl:(NSURL*)url withError:(NSError**)error
{
    // get the query string parameters
    NSArray* pairs = [url.query componentsSeparatedByString:@"&"];
    if (pairs.count == 0)
    {
        if(error) *error = [TMMeasuredPhoto getErrorForCode:TMMeasuredPhotoErrorCodeInvalidResponse];
        return nil;
    }
    NSMutableDictionary* params = [[NSMutableDictionary alloc] initWithCapacity:pairs.count];
    for (NSString* pair in pairs)
    {
        NSArray* keyAndValue = [pair componentsSeparatedByString:@"="];
        if (keyAndValue.count != 2) continue;
        params[keyAndValue[0]] = keyAndValue[1];
    }
    if (params.count == 0)
    {
        if(error) *error = [TMMeasuredPhoto getErrorForCode:TMMeasuredPhotoErrorCodeInvalidResponse];
        return nil;
    }
    
    // check "host", which we define as the action the url is requesting
    NSString* action = url.host;
    
    if ([action isEqualToString:kTMUrlActionMeasuredPhoto])
    {
        NSString* pasteboardId = params[kTMQueryStringPasteboard];
        if (pasteboardId == nil || pasteboardId.length == 0)
        {
            if(error) *error = [TMMeasuredPhoto getErrorForCode:TMMeasuredPhotoErrorCodeInvalidResponse];
            return nil;
        }
        else
        {
            // in the future, if there are multiple api versions, we will check the version of the response here.
            return [TMMeasuredPhoto getMeasuredPhotoFromPasteboard:pasteboardId withError:error];
        }
    }
    else if ([action isEqualToString:kTMUrlActionError])
    {
        NSString* errorCodeString = params[kTMQueryStringErrorCode];
        if (errorCodeString == nil || errorCodeString.length == 0)
        {
            if(error) *error = [TMMeasuredPhoto getErrorForCode:TMMeasuredPhotoErrorCodeInvalidResponse];
            return nil;
        }
        else
        {
            int errorCode = [errorCodeString intValue];
            if(error) *error = [TMMeasuredPhoto getErrorForCode:errorCode];
            return nil;
        }
    }
    else
    {
        if(error) *error = [TMMeasuredPhoto getErrorForCode:TMMeasuredPhotoErrorCodeInvalidResponse];
        return nil;
    }
}

+ (TMMeasuredPhoto*) getMeasuredPhotoFromPasteboard:(NSString*)pasteboardId withError:(NSError**)error
{
    UIPasteboard* pasteboard = [UIPasteboard pasteboardWithName:pasteboardId create:NO];
    if (pasteboard)
    {
        NSData *data = [pasteboard dataForPasteboardType:kTMMeasuredPhotoUTI];
        if (data)
        {
            return [TMMeasuredPhoto unarchivePackageData:data];
        }
        else
        {
            if(error) *error = [TMMeasuredPhoto getErrorForCode:TMMeasuredPhotoErrorCodePasteboard];
        }
        
        // clean up the pasteboard
        [pasteboard setData:nil forPasteboardType:kTMMeasuredPhotoUTI];
        [pasteboard setPersistent:NO];
        
        return nil;
    }
    else
    {
        if(error) *error = [TMMeasuredPhoto getErrorForCode:TMMeasuredPhotoErrorCodePasteboard];
        return nil;
    }
}

+ (NSError*) getErrorForCode:(TMMeasuredPhotoErrorCode)code
{
    NSString* errorDesc;
    
    switch (code) {
        case TMMeasuredPhotoErrorCodeMissingApiKey:
            errorDesc = @"The API key was missing or zero length";
            break;
        case TMMeasuredPhotoErrorCodeLicenseValidationFailure:
            errorDesc = @"Failed to validate the license. Make sure an Internet conenction is available.";
            break;
        case TMMeasuredPhotoErrorCodeWrongLicenseType:
            errorDesc = @"The supplied API key has the wrong license type. Please use an API key with a TrueMeasure license type.";
            break;
        case TMMeasuredPhotoErrorCodeInvalidAction:
            errorDesc = @"An invalid action was specified in the URL. The action is defined as the 'host' part of the URL.";
            break;
        case TMMeasuredPhotoErrorCodeLicenseInvalid:
            errorDesc = @"The API key is invalid.";
            break;
        case TMMeasuredPhotoErrorCodeInvalidResponse:
            errorDesc = @"An invalid response was received from TrueMeasure. Please report this to RealityCap.";
            break;
        case TMMeasuredPhotoErrorCodePasteboard:
            errorDesc = @"A pasteboard related error occurred. Please report this to RealityCap.";
            break;
        case TMMeasuredPhotoErrorCodeUnknown:
            errorDesc = @"An unknown error occurred. Please report this to RealityCap.";
            break;
        default:
            break;
    }
    
    NSDictionary* userInfo;
    if (errorDesc) userInfo = @{NSLocalizedDescriptionKey: errorDesc};
    
    return [NSError errorWithDomain:ERROR_DOMAIN code:code userInfo:userInfo];
}

#pragma mark - NSCoding

+ (BOOL) supportsSecureCoding
{
    return YES;
}

- (void) encodeWithCoder:(NSCoder *)encoder
{
    [encoder encodeObject:self.appVersion forKey:kTMKeyAppVersion];
    [encoder encodeObject:self.appBuildNumber forKey:kTMKeyAppBuildNumber];
    [encoder encodeObject:self.featurePoints forKey:kTMKeyFeaturePoints];
    [encoder encodeObject:self.imageData forKey:kTMKeyImageData];
}

- (id) initWithCoder:(NSCoder *)decoder
{
    if (self = [super init])
    {
        [TMFeaturePoint class]; // don't know why this is necessary, but feature points won't decode without it
        _appVersion = [decoder decodeObjectOfClass:[NSString class] forKey:kTMKeyAppVersion];
        _appBuildNumber = [decoder decodeObjectOfClass:[NSNumber class] forKey:kTMKeyAppBuildNumber];
        _featurePoints = [decoder decodeObjectOfClass:[NSArray class] forKey:kTMKeyFeaturePoints];
        _imageData = [decoder decodeObjectOfClass:[NSData class] forKey:kTMKeyImageData];
    }
    return self;
}

#pragma mark - Data Helpers

+ (TMMeasuredPhoto*) unarchivePackageData:(NSData *)data
{
    NSKeyedUnarchiver *unarchiver = [[NSKeyedUnarchiver alloc] initForReadingWithData:data];
    TMMeasuredPhoto *measuredPhoto = [unarchiver decodeObjectOfClass:[TMMeasuredPhoto class] forKey:kTMKeyMeasuredPhotoData];
    [unarchiver finishDecoding];
    return measuredPhoto;
}

@end
