//
//  TMMeasuredPhoto.h
//  TrueMeasureSDK
//
//  Created by Ben Hirashima on 10/15/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

extern NSString *kTMMeasuredPhotoUTI;

static int kTMApiVersion = 1;
static NSString* kTMQueryStringApiKey = @"apikey";

typedef NS_ENUM(int, TMMeasuredPhotoErrorCode)
{
    TMMeasuredPhotoErrorCodeMissingApiKey = 100,
    TMMeasuredPhotoErrorCodeLicenseValidationFailure = 200,
    TMMeasuredPhotoErrorCodeLicenseInvalid = 300,
    TMMeasuredPhotoErrorCodeWrongLicenseType = 400,
    TMMeasuredPhotoErrorCodeInvalidAction = 500,
    TMMeasuredPhotoErrorCodePasteboard = 600,
    TMMeasuredPhotoErrorCodeInvalidResponse = 700,
    TMMeasuredPhotoErrorCodeUnknown = 800
};

@interface TMMeasuredPhoto : NSObject <NSCoding>

@property (nonatomic) NSString* appVersion;
@property (nonatomic) NSNumber* appBuildNumber;

/** 
 Checks to see if TrueMeasure is installed, and what API versions it supports. Note that API version is not the same as 
 the version of the TrueMeasure app itself.
 @returns The highest API version supported by the version of TrueMeasure that is installed. Returns zero if TrueMeasure
 is not installed.
 */
+ (int) getHighestInstalledApiVersion;
+ (BOOL) requestMeasuredPhoto:(NSString*)apiKey;
+ (BOOL) requestMeasuredPhoto:(NSString*)apiKey withApiVersion:(int)apiVersion;
+ (TMMeasuredPhoto*) retrieveFromUrl:(NSURL *)url withError:(NSError**)error;
- (NSData*) dataRepresentation;

@end
