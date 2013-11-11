//
//  TMMeasuredPhoto.h
//  TrueMeasureSDK
//
//  Created by Ben Hirashima on 10/15/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "TMFeaturePoint.h"

extern NSString *kTMMeasuredPhotoUTI;

static int const kTMApiVersion = 1;
static NSString* const kTMKeyMeasuredPhotoData = @"kTMKeyMeasuredPhotoData";

static NSString* const kTMUrlActionMeasuredPhoto = @"measuredphoto";
static NSString* const kTMUrlActionError = @"error";

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

@interface TMMeasuredPhoto : NSObject <NSSecureCoding>

/** The version of TrueMeasure that produced this measured photo */
@property (nonatomic) NSString* appVersion;
/** The build number of TrueMeasure */
@property (nonatomic) NSNumber* appBuildNumber;
/** A array of TMFeaturePoints representing the tappable features in the measured photo. */
@property (nonatomic) NSArray* featurePoints;
/** The photo data */
@property (nonatomic) NSData* imageData;

/** 
 Checks to see if TrueMeasure is installed, and what API versions it supports. Note that the API version is not the same as
 the version of the TrueMeasure app itself.
 @returns The highest API version supported by the version of TrueMeasure that is installed. Returns zero if TrueMeasure
 is not installed.
 */
+ (int) getHighestInstalledApiVersion;

/**
 Requests a measured photo from TrueMeasure. The requested API version defaults to the highest version supported by the current SDK.
 You should check getHighestInstalledApiVersion and compare it to the current version (kTMApiVersion) before calling this method.
 The measured photo is sent back via a call to a custom URL scheme implemented by your app.
 The URL scheme must begin with your app's bundle ID, and be followed by ".truemeasure.measuredphoto". So if your app's bundle ID is 
 "com.realitycap.SampleApp", then your custom URL scheme would be "com.realitycap.SampleApp.truemeasure.measuredphoto". 
 See Apple's documentation on custom URL schemes for more info. https://developer.apple.com/library/ios/documentation/iphone/conceptual/iphoneosprogrammingguide/AdvancedAppTricks/AdvancedAppTricks.html#//apple_ref/doc/uid/TP40007072-CH7-SW18
 @param apiKey The API key provided to you by RealityCap.
 @returns YES if request was sent, NO if TrueMeasure is not installed, or requested API version not supported.
 */
+ (BOOL) requestMeasuredPhoto:(NSString*)apiKey;

/**
 The same as requestMeasuredPhoto:, but explictly sets the API version. Use this if you want to request a measured photo that conforms
 to a specific version of the API. You should check getHighestInstalledApiVersion before calling this method to make sure the API version
 is supported by the currently installed version of TrueMeasure.
 */
+ (BOOL) requestMeasuredPhoto:(NSString*)apiKey withApiVersion:(int)apiVersion;

/**
 After the measured photo has been requested, and TrueMeasure has produced it, TrueMeasure will make a call to your custom URL.
 If your app has specified that it supports the custom URL (see requestMeasuredPhoto:), your app will be launched, and the URL
 will be passed to your AppDelegate via the method application:openURL:sourceApplication:annotation:. You must implement that method
 and then pass in the URL to this method, along with a pointer to an error object, like so:
 
 - (BOOL)application:(UIApplication *)application openURL:(NSURL *)url sourceApplication:(NSString *)sourceApplication annotation:(id)annotation
 {
    if ([sourceApplication isEqualToString:kTMTrueMeasureBundleId]) // Make sure the request actually came from TrueMeasure. Very important for security!
    {
        if ([url.host isEqualToString:kTMUrlActionMeasuredPhoto]) // Check if this is a measured photo URL by checking the "host" part of the URL
        {
            NSError* error;
            TMMeasuredPhoto* measuredPhoto = [TMMeasuredPhoto retrieveMeasuredPhotoWithUrl:url withError:&error]; // Make sure you pass a pointer to the error, not the error object itself

            if (error) // If an error occurred, error will be non-nil.
            {
                UIAlertView *alert = [[UIAlertView alloc] initWithTitle:[NSString stringWithFormat:@"Error %i", error.code]
                                                               message:error.localizedDescription
                                                              delegate:self
                                                     cancelButtonTitle:nil
                                                     otherButtonTitles:@"OK", nil];
                [alert show];
                return NO;
            }

            if (measuredPhoto)
            {
                // Success. Use the TMMeasuredPhoto object as you will.
                return YES;
            }
        }
    }
 }
 
 @param url The URL object supplied by application:openURL:sourceApplication:annotation:.
 @param error A pointer to an NSError variable. Make sure you pass a pointer, like &error. If an error occurs, the 'code' property
 of the error object will correspond to one of the values from the TMMeasuredPhotoErrorCode enum.
 @returns An instance of TMMeasuredPhoto, containing a JPG image and a collection of measured features from the image.
 */
+ (TMMeasuredPhoto*) retrieveMeasuredPhotoWithUrl:(NSURL *)url withError:(NSError**)error;

@end
