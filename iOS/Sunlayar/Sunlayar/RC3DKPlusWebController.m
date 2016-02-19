//
//  RC3DKPlusWebController.m
//  Sunlayar
//
//  Created by Ben Hirashima on 2/25/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#import "RC3DKPlusWebController.h"
#import "RC3DK.h"
#import "SLMeasuredPhoto.h"
#import "RCDebugLog.h"

@interface RC3DKPlusWebController ()

@end

@implementation RC3DKPlusWebController
{
    RCSensorFusionData* lastSensorFusionDataWithImage;
}
@synthesize measuredPhoto;

- (void)viewDidLoad
{
    [super viewDidLoad];
    _isStereoRunning = NO;
}

#pragma mark - Stereo stuff

- (BOOL) startStereoCapture
{
    if (!self.isSensorFusionRunning) return NO;
    
    [RCStereo.sharedInstance reset];
    _isStereoRunning = YES;
    
    return YES;
}

- (NSString*) finishStereoCapture
{
    _isStereoRunning = NO;
    
    measuredPhoto = [SLMeasuredPhoto new];
    
    RCStereo * stereo = [RCStereo sharedInstance];
    stereo.delegate = self;
    [stereo setWorkingDirectory:WORKING_DIRECTORY_URL andGuid:measuredPhoto.id_guid andOrientation:UIDeviceOrientationLandscapeLeft];
    [stereo processFrame:lastSensorFusionDataWithImage withFinal:true];
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
        [measuredPhoto writeImagetoJpeg:lastSensorFusionDataWithImage.sampleBuffer withOrientation:stereo.orientation];
        [stereo preprocess];
    });
    
    return measuredPhoto.id_guid;
}

- (void) cancelStereoCapture
{
    [RCStereo.sharedInstance reset];
    _isStereoRunning = NO;
}

#pragma mark - RCStereoDelegate

- (void) stereoDidUpdateProgress:(float)progress
{
    NSString* javascript = [NSString stringWithFormat:@"RC3DK.stereoDidUpdateProgress(%f);", progress];
//    DLog(@"%@", javascript);
    [self.webView stringByEvaluatingJavaScriptFromString: javascript];
}

- (void) stereoDidFinish
{
    LOGME
    [self.webView stringByEvaluatingJavaScriptFromString: @"RC3DK.stereoDidFinish();"];
}

#pragma mark - RCSensorFusionDelegate

//- (void) sensorFusionDidChangeStatus:(RCSensorFusionStatus *)status
//{
//    [super sensorFusionDidChangeStatus:status];
//}

- (void) sensorFusionDidUpdateData:(RCSensorFusionData*)data
{
    [super sensorFusionDidUpdateData:data];
    
    if (data.sampleBuffer)
    {
        lastSensorFusionDataWithImage = data;
        if (self.isStereoRunning) [RCStereo.sharedInstance processFrame:data withFinal:false];
    }
}

#pragma mark - RCHttpInterceptorDelegate

- (NSDictionary *)handleAction:(ARNativeAction *)nativeAction error:(NSError **)error
{
    // allow superclass to handle this action. if it doesn't, then handle it ourselves.
    NSDictionary* result = [super handleAction:nativeAction error:error];
    if (result) return result;
    
    if ([nativeAction.request.URL.relativePath isEqualToString:@"/startStereoCapture"])
    {
        return @{ @"result": @([self startStereoCapture]) };
    }
    else if ([nativeAction.request.URL.relativePath isEqualToString:@"/finishStereoCapture"])
    {
        return @{ @"result": [self finishStereoCapture] };
    }
    else if ([nativeAction.request.URL.relativePath isEqualToString:@"/cancelStereoCapture"])
    {
        [self cancelStereoCapture];
        return @{ @"result": @YES };
    }
    
    return nil;
}

@end
