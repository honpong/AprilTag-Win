//
//  MPPhotoRequest.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 10/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "MPPhotoRequest.h"
#import <RC3DK/RC3DK.h>
#import <TrueMeasureSDK/TrueMeasureSDK.h>

static NSString* const kTMQueryStringApiKey = @"apikey";

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
        if (![action isEqualToString:@"measuredphoto"])
        {
            [self returnErrorToCallingApp:TMMeasuredPhotoErrorCodeInvalidAction];
            return self;
        }
        
        // check the query string
        NSArray* pairs = [url.query componentsSeparatedByString:@"&"];
        if (pairs.count == 0)
        {
            [self returnErrorToCallingApp:TMMeasuredPhotoErrorCodeMissingApiKey];
            return self;
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
            [self returnErrorToCallingApp:TMMeasuredPhotoErrorCodeMissingApiKey];
            return self;
        }
        
        apiKey = params[kTMQueryStringApiKey];
        if (apiKey == nil || apiKey.length == 0 || [apiKey isEqualToString:@"(null)"])
        {
            [self returnErrorToCallingApp:TMMeasuredPhotoErrorCodeMissingApiKey];
            return self;
        }
        
        isLicenseValid = NO;
        __weak MPPhotoRequest* weakSelf = self;
        //TODO - implement truemeasure API license
        [SENSOR_FUSION
         validateLicense:apiKey
         withCompletionBlock:^(int licenseType, int licenseStatus) {
             if (licenseStatus == 0)
             {
                 if (licenseType == 0) // TODO truemeasure license type
                 {
                     isLicenseValid = YES;
                 }
                 else
                 {
                     [weakSelf returnErrorToCallingApp:TMMeasuredPhotoErrorCodeWrongLicenseType];
                 }
             }
             else
             {
                 [weakSelf returnErrorToCallingApp:TMMeasuredPhotoErrorCodeLicenseInvalid];
             }
         }
         withErrorBlock:^(NSError *error) {
             DLog(@"License validation failure: %@", error);
             [weakSelf returnErrorToCallingApp:TMMeasuredPhotoErrorCodeLicenseValidationFailure];
         }];
    }
    
    return self;
}

- (void) returnErrorToCallingApp:(TMMeasuredPhotoErrorCode)code
{
    NSString* urlString = [NSString stringWithFormat:@"%@.truemeasure.measuredphoto://error?code=%i", sourceApp, code];
    NSURL *myURL = [NSURL URLWithString:urlString];
    if ([[UIApplication sharedApplication] canOpenURL:myURL]) [[UIApplication sharedApplication] openURL:myURL];
    isRepliedTo = YES;
}

- (NSData*) dataRepresentation:(TMMeasuredPhoto*)measuredPhoto
{
    NSMutableData *data = [[NSMutableData alloc] init];
    NSKeyedArchiver *archiver = [[NSKeyedArchiver alloc] initForWritingWithMutableData:data];
    [archiver encodeObject:measuredPhoto forKey:kTMKeyMeasuredPhotoData];
    [archiver finishEncoding];
    
    return [NSData dataWithData:data];
}

- (BOOL) sendMeasuredPhoto:(TMMeasuredPhoto*)measuredPhoto
{
    if (isLicenseValid)
    {
        UIPasteboard *pasteboard = [UIPasteboard pasteboardWithUniqueName];
        [pasteboard setPersistent:YES];
        [pasteboard setData:[self dataRepresentation:measuredPhoto] forPasteboardType:kTMMeasuredPhotoUTI];
        
        NSString* urlString = [NSString stringWithFormat:@"%@.truemeasure.measuredphoto://measuredphoto/v1?pasteboard=%@", sourceApp, pasteboard.name];
        NSURL *myURL = [NSURL URLWithString:urlString];
        if ([[UIApplication sharedApplication] canOpenURL:myURL])
        {
            [[UIApplication sharedApplication] openURL:myURL];
            isRepliedTo = YES;
            return YES;
        }
        else
        {
            return NO;
        }
    }
    else
    {
        return NO;
    }
}

+ (NSArray*) transcribeFeaturePoints:(NSArray*)featurePoints
{
    if (featurePoints == nil)
    {
        DLog(@"WARNING: featurePoints is nil");
        return nil;
    }
    
    NSMutableArray* outputPoints = [NSMutableArray arrayWithCapacity:featurePoints.count];
    
    for (RCFeaturePoint* rcPoint in featurePoints)
    {
        TMScalar* originalDepth = [[TMScalar alloc] initWithScalar:rcPoint.originalDepth.scalar withStdDev:rcPoint.originalDepth.standardDeviation];
        TMPoint* worldPoint = [[TMPoint alloc] initWithVector:rcPoint.worldPoint.vector withStandardDeviation:rcPoint.worldPoint.standardDeviation];
        TMFeaturePoint* tmPoint = [[TMFeaturePoint alloc] initWithX:rcPoint.x
                                                              withY:rcPoint.y
                                                  withOriginalDepth:originalDepth
                                                     withWorldPoint:worldPoint];
        [outputPoints addObject:tmPoint];
    }
    
    return [NSArray arrayWithArray:outputPoints];
}

+ (NSData*) sampleBufferToNSData:(CMSampleBufferRef)sampleBuffer
{
    if (sampleBuffer)
    {
        sampleBuffer = (CMSampleBufferRef)CFRetain(sampleBuffer);
    }
    else
    {
        return nil;
    }
    
    //inside RCSensorFusionData we have a CMSampleBufferRef from which we can get CVImageBufferRef
    CVImageBufferRef pixBuf = CMSampleBufferGetImageBuffer(sampleBuffer);
    //we then need to turn that CVImageBuffer into an UIImage, which can be written to NSData as a JPG
    CIImage *ciImage = [CIImage imageWithCVPixelBuffer:pixBuf];
    
    CIContext *temporaryContext = [CIContext contextWithOptions:nil];
    CGImageRef videoImage = [temporaryContext
                             createCGImage:ciImage
                             fromRect:CGRectMake(0, 0,
                                                 CVPixelBufferGetWidth(pixBuf),
                                                 CVPixelBufferGetHeight(pixBuf))];
    
    UIImage *uiImage = [UIImage imageWithCGImage:videoImage];
    CFRelease(videoImage);
    CFRelease(sampleBuffer);
    return UIImageJPEGRepresentation(uiImage, 0.6);
}

@end
