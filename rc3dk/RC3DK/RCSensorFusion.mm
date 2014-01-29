//
//  RCSensorFusion.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCSensorFusion.h"
extern "C" {
#import "cor.h"
}
#include "filter_setup.h"
#include <mach/mach_time.h>
#import "RCCalibration.h"
#import "RCCameraManager.h"
#import "RCPrivateHTTPClient.h"
#import "NSString+RCString.h"

uint64_t get_timestamp()
{
    static mach_timebase_info_data_t s_timebase_info;
    if (s_timebase_info.denom == 0) {
        mach_timebase_info(&s_timebase_info);
    }
    return mach_absolute_time() * s_timebase_info.numer / s_timebase_info.denom / 1000;
}


@interface RCSensorFusionOperation : NSObject

@property (strong) void (^block)();
@property uint64_t time;

- (id) initWithBlock:(void (^)(void))block withTime:(uint64_t)time;

@end

@implementation RCSensorFusionOperation

@synthesize block;
@synthesize time;

- (id) initWithBlock:(void (^)())block_ withTime:(uint64_t)time_
{
    if(self = [super init]) {
        block = block_;
        time = time_;
    }
    return self;
}

@end

@interface RCSensorFusion () <RCCameraManagerDelegate>
@end

@implementation RCSensorFusion
{
    filter_setup *_cor_setup;
    bool isSensorFusionRunning;
    bool isProcessingVideo;
    bool processingVideoRequested;
    bool didReset;
    CVPixelBufferRef pixelBufferCached;
    dispatch_queue_t queue, inputQueue;
    NSMutableArray *dataWaiting;
    uint64_t lastCallbackTime;
    BOOL isLicenseValid;
}

#define minimumCallbackInterval 100000
#define SKIP_LICENSE_CHECK YES // do not change the name of this macro without also changing the framework build script that looks for it

- (void) validateLicense:(NSString*)apiKey withCompletionBlock:(void (^)(int, int))completionBlock withErrorBlock:(void (^)(NSError*))errorBlock
{
    if (apiKey == nil || apiKey.length == 0)
    {
        if (errorBlock) errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorApiKeyMissing userInfo:[NSDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. API key was nil or zero length.", NSLocalizedDescriptionKey, @"API key was nil or zero length.", NSLocalizedFailureReasonErrorKey, nil]]);
        return;
    }
    
    NSString* bundleId = [[NSBundle mainBundle] bundleIdentifier];
//    bundleId = @"com.realitycap.tapemeasure"; // for running unit tests only. getting bundle id doesn't work while running tests.
    if (bundleId == nil || bundleId.length == 0)
    {
        if (errorBlock) errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorBundleIdMissing userInfo:[NSDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. Could not get bundle ID.", NSLocalizedDescriptionKey, @"Could not get bundle ID.", NSLocalizedFailureReasonErrorKey, nil]]);
        return;
    }

    NSString* vendorId = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
    if (vendorId == nil || vendorId.length == 0)
    {
        if (errorBlock) errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorVendorIdMissing userInfo:[NSDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. Could not get ID for vendor.", NSLocalizedDescriptionKey, @"Could not get ID for vendor.", NSLocalizedFailureReasonErrorKey, nil]]);
        return;
    }

    NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:
                            @"3DK_TR", @"requested_resource",
                            apiKey, @"api_key",
                            bundleId, @"bundle_id",
                            vendorId, @"vendor_id",
                            nil];
    
    RCPrivateHTTPClient*client = [RCPrivateHTTPClient sharedInstance];
    
    [client
     postPath:API_LICENSING_POST
     parameters:params
     success:^(RCAFHTTPRequestOperation *operation, id JSON)
     {
         DLog(@"License completion %i\n%@", operation.response.statusCode, operation.responseString);
         if (operation.response.statusCode != 200)
         {
             if (errorBlock) errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorHttpError userInfo:[NSDictionary dictionaryWithObjectsAndKeys:[NSString stringWithFormat:@"Failed to validate license. HTTP response code %i.", operation.response.statusCode], NSLocalizedDescriptionKey, [NSString stringWithFormat:@"HTTP status %i: %@", operation.response.statusCode, operation.responseString], NSLocalizedFailureReasonErrorKey, nil]]);
             return;
         }
         
         if (JSON == nil)
         {
             if (errorBlock) errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorEmptyResponse userInfo:[NSDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. Response body was empty.", NSLocalizedDescriptionKey, @"Response body was empty.", NSLocalizedFailureReasonErrorKey, nil]]);
             return;
         }
         
         NSError* serializationError;
         NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:&serializationError];
         if (serializationError || response == nil)
         {
             if (errorBlock)
             {
                 NSMutableDictionary* userInfo = [NSMutableDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. Failed to deserialize response.", NSLocalizedDescriptionKey, @"Failed to deserialize response.", NSLocalizedFailureReasonErrorKey, nil];
                 if (serializationError) [userInfo setObject:serializationError forKey:NSUnderlyingErrorKey];
                 errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorDeserialization userInfo:userInfo]);
             }
             return;
         }
         
         NSNumber* licenseStatus = [response objectForKey:@"license_status"];
         NSNumber* licenseType = [response objectForKey:@"license_type"];
         
         if (licenseStatus == nil || licenseType == nil)
         {
             if (errorBlock) errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorInvalidResponse userInfo:[NSDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. Invalid response from server.", NSLocalizedDescriptionKey, @"Invalid response from server.", NSLocalizedFailureReasonErrorKey, nil]]);
             return;
         }
         
         if ([licenseStatus intValue] == RCLicenseStatusOK) isLicenseValid = YES; // TODO: handle other license types
         
         if (completionBlock) completionBlock([licenseType intValue], [licenseStatus intValue]);
     }
     failure:^(RCAFHTTPRequestOperation *operation, NSError *error)
     {
         DLog(@"License failure: %i\n%@", operation.response.statusCode, operation.responseString);
         if (errorBlock)
         {
             NSMutableDictionary* userInfo = [NSMutableDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. HTTPS request failed.", NSLocalizedDescriptionKey, @"HTTPS request failed. See underlying error.", NSLocalizedFailureReasonErrorKey, nil];
             if (error) [userInfo setObject:error forKey:NSUnderlyingErrorKey];
             errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorHttpFailure userInfo:userInfo]);
         }
     }
     ];
}

- (void) enqueueOperation:(RCSensorFusionOperation *)operation
{
    int index;
    for(index = 0; index < [dataWaiting count]; ++index) {
        if(operation.time < ((RCSensorFusionOperation *)dataWaiting[index]).time) break;
    }
    [dataWaiting insertObject:operation atIndex:index];
}

- (void) flushOperationsBeforeTime:(uint64_t)time
{
    while([dataWaiting count]) {
        RCSensorFusionOperation *op = (RCSensorFusionOperation *)dataWaiting[0];
        if(op.time >= time) break;
        dispatch_async(queue, op.block);
        [dataWaiting removeObjectAtIndex:0];
    }
}

+ (id) sharedInstance
{
    static RCSensorFusion *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [[self alloc] init];
    });
    return instance;
}

- (id) init
{
    self = [super init];
    
    if (self)
    {
        LOGME
        isLicenseValid = NO;
        isSensorFusionRunning = NO;
        isProcessingVideo = NO;
        didReset = false;
        dataWaiting = [NSMutableArray arrayWithCapacity:10];
        queue = dispatch_queue_create("com.realitycap.sensorfusion", DISPATCH_QUEUE_SERIAL);
        inputQueue = dispatch_queue_create("com.realitycap.sensorfusion.input", DISPATCH_QUEUE_SERIAL);
        lastCallbackTime = 0;
        
        [RCPrivateHTTPClient initWithBaseUrl:API_BASE_URL withAcceptHeader:API_HEADER_ACCEPT withApiVersion:API_VERSION];
    }
    
    return self;
}

- (void) dealloc
{
    if (pixelBufferCached)
    {
        CVPixelBufferUnlockBaseAddress(pixelBufferCached, kCVPixelBufferLock_ReadOnly);
        CVPixelBufferRelease(pixelBufferCached);
    }
}

- (void) startInertialOnlyFusion
{
    LOGME
    if(isSensorFusionRunning) return;
    
    corvis_device_parameters dc = [RCCalibration getCalibrationData];
    _cor_setup = new filter_setup(&dc);

    cor_time_init();
    plugins_start();
    isSensorFusionRunning = true;
}

- (void) setLocation:(CLLocation*)location
{
    if(location)
    {
        dispatch_async(queue, ^{ filter_compute_gravity(&_cor_setup->sfm, location.coordinate.latitude, location.altitude); } );
    }
}

- (bool) hasCalibrationData
{
    return [RCCalibration hasCalibrationData];
}

- (void) startStaticCalibration
{
    dispatch_async(queue, ^{
        filter_start_static_calibration(&_cor_setup->sfm);
    });
}

- (void) stopStaticCalibration
{
    dispatch_async(queue, ^{
        filter_stop_static_calibration(&_cor_setup->sfm);
    });
    [self saveCalibration];
}

- (void) focusOperationFinished:(bool)timedOut
{
    // startProcessingVideo
    if(processingVideoRequested && !isProcessingVideo) {
        dispatch_async(queue, ^{
            filter_start_processing_video(&_cor_setup->sfm);
        });
        isProcessingVideo = true;
        processingVideoRequested = false;
    }
}

- (void) startProcessingVideoWithDevice:(AVCaptureDevice *)device
{
    if(isProcessingVideo) return;
    if (SKIP_LICENSE_CHECK || isLicenseValid)
    {
        isLicenseValid = NO; // evaluation license must be checked every time. need more logic here for other license types.
        RCCameraManager * cameraManager = [RCCameraManager sharedInstance];

        processingVideoRequested = YES;
        cameraManager.delegate = self;
        [cameraManager setVideoDevice:device];
        [cameraManager lockFocus];
    }
    else if ([self.delegate respondsToSelector:@selector(sensorFusionError:)])
    {
        NSDictionary* userInfo = [NSDictionary dictionaryWithObjectsAndKeys:@"Cannot start sensor fusion. License needs to be validated.", NSLocalizedDescriptionKey, nil];
        NSError *error =[[NSError alloc] initWithDomain:ERROR_DOMAIN code:RCSensorFusionErrorCodeLicense userInfo:userInfo];
        [self.delegate sensorFusionError:error];
    }
}

- (void) stopProcessingVideo
{
    if(!isProcessingVideo && !processingVideoRequested) return;

    RCCameraManager * cameraManager = [RCCameraManager sharedInstance];

    dispatch_async(queue, ^{
        filter_stop_processing_video(&_cor_setup->sfm);
        [RCCalibration postDeviceCalibration:nil onFailure:nil];
    });
    [cameraManager releaseVideoDevice];
    isProcessingVideo = false;
    processingVideoRequested = false;
}

- (void) selectUserFeatureWithX:(float)x withY:(float)y
{
    if(!isProcessingVideo) return;
    dispatch_async(queue, ^{ filter_select_feature(&_cor_setup->sfm, x, y); });
}

- (void) stopSensorFusion
{
    LOGME
    if(!isSensorFusionRunning) return;
    
    dispatch_sync(inputQueue, ^{
        isSensorFusionRunning = false;
        isProcessingVideo = false;
        
        [self saveCalibration];
        dispatch_sync(queue, ^{});

        plugins_stop();
        [self teardownPlugins];
    });
}

- (void) filterCallbackWithSampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    if (sampleBuffer) sampleBuffer = (CMSampleBufferRef)CFRetain(sampleBuffer);
    
    //perform these operations synchronously in the calling (filter) thread
    int failureCode = _cor_setup->get_failure_code();
    struct filter *f = &(_cor_setup->sfm);
    float converged = _cor_setup->get_filter_converged();

    bool
        steady = _cor_setup->get_device_steady(),
        visionfail = _cor_setup->get_vision_failure(),
        speedfail = _cor_setup->get_speed_failure(),
        otherfail = _cor_setup->get_other_failure();
    
    int errorCode = 0;
    if (otherfail)
        errorCode = RCSensorFusionErrorCodeOther;
    else if(speedfail)
        errorCode = RCSensorFusionErrorCodeTooFast;
    else if (visionfail)
        errorCode = RCSensorFusionErrorCodeVision;
        
    RCSensorFusionStatus* status = [[RCSensorFusionStatus alloc] initWithProgress:converged withStatusCode:failureCode withIsSteady:steady];
    RCTranslation* translation = [[RCTranslation alloc] initWithVector:f->s.T.v withStandardDeviation:v4_sqrt(f->s.T.variance)];
    RCRotation* rotation = [[RCRotation alloc] initWithVector:f->s.W.v withStandardDeviation:v4_sqrt(f->s.W.variance)];
    RCTransformation* transformation = [[RCTransformation alloc] initWithTranslation:translation withRotation:rotation];

    RCTranslation* camT = [[RCTranslation alloc] initWithVector:f->s.Tc.v withStandardDeviation:v4_sqrt(f->s.Tc.variance)];
    RCRotation* camR = [[RCRotation alloc] initWithVector:f->s.Wc.v withStandardDeviation:v4_sqrt(f->s.Wc.variance)];
    RCTransformation* camTransform = [[RCTransformation alloc] initWithTranslation:camT withRotation:camR];
    
    RCScalar *totalPath = [[RCScalar alloc] initWithScalar:f->s.total_distance withStdDev:0.];
    
    RCCameraParameters *camParams = [[RCCameraParameters alloc] initWithFocalLength:f->s.focal_length.v withOpticalCenterX:f->s.center_x.v withOpticalCenterY:f->s.center_y.v withRadialSecondDegree:f->s.k1.v withRadialFourthDegree:f->s.k2.v];

    RCSensorFusionData* data = [[RCSensorFusionData alloc] initWithStatus:status withTransformation:transformation withCameraTransformation:[transformation composeWithTransformation:camTransform] withCameraParameters:camParams withTotalPath:totalPath withFeatures:[self getFeaturesArray] withSampleBuffer:sampleBuffer withTimestamp:f->last_time];

    //send the callback to the main/ui thread
    dispatch_async(dispatch_get_main_queue(), ^{
        if ([self.delegate respondsToSelector:@selector(sensorFusionDidUpdate:)]) [self.delegate sensorFusionDidUpdate:data];
        if (errorCode && [self.delegate respondsToSelector:@selector(sensorFusionError:)])
        {
            NSError *error =[[NSError alloc] initWithDomain:ERROR_DOMAIN code:errorCode userInfo:nil];
            [self.delegate sensorFusionError:error];
            if(speedfail || otherfail || (visionfail && !_cor_setup->sfm.active)) {
                // If we haven't yet started and we have vision failures, refocus
                if(visionfail && !_cor_setup->sfm.active) {
                    // Switch back to inertial only mode and wait for focus to finish
                    dispatch_async(queue, ^{
                        filter_stop_processing_video(&_cor_setup->sfm);
                    });
                }
                else {
                    // Do a full filter reset and wait for focus to finish
                    dispatch_async(queue, ^{
                        filter_reset_full(&_cor_setup->sfm);
                    });
                }

                isProcessingVideo = false;
                processingVideoRequested = true;

                // Request a refocus
                RCCameraManager * cameraManager = [RCCameraManager sharedInstance];
                [cameraManager focusOnceAndLock];
            }
        }
        if(sampleBuffer) CFRelease(sampleBuffer);
    });
    lastCallbackTime = get_timestamp();
}

//needs to be called from the filter thread
- (NSArray*) getFeaturesArray
{
    struct filter *f = &(_cor_setup->sfm);
    NSMutableArray* array = [[NSMutableArray alloc] initWithCapacity:f->s.features.size()];
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        state_vision_feature *i = *fiter;
        if(i->status == feature_normal || i->status == feature_initializing || i->status == feature_ready) {
            f_t logstd = sqrt(i->variance);
            f_t rho = exp(i->v);
            f_t drho = exp(i->v + logstd);
            f_t stdev = drho - rho;
            
            RCFeaturePoint* feature = [[RCFeaturePoint alloc] initWithId:i->id withX:i->current[0] withY:i->current[1] withOriginalDepth:[[RCScalar alloc] initWithScalar:exp(i->v) withStdDev:stdev] withWorldPoint:[[RCPoint alloc]initWithX:i->world[0] withY:i->world[1] withZ:i->world[2]] withInitialized:(i->status == feature_normal)];
            [array addObject:feature];
        }
    }
    return [NSArray arrayWithArray:array];
}

- (void) teardownPlugins
{
    LOGME
    if(_cor_setup) delete _cor_setup;
    plugins_clear();
}

- (BOOL) isSensorFusionRunning
{
    return isSensorFusionRunning;
}

- (BOOL) isProcessingVideo
{
    return isProcessingVideo;
}

- (void) resetOrigin
{
    LOGME
    dispatch_async(queue, ^{
        filter_set_reference(&_cor_setup->sfm);
    });
}

- (bool) saveCalibration
{
    LOGME
    __block struct corvis_device_parameters finalDeviceParameters;
    __block bool parametersGood;
    dispatch_sync(queue, ^{
        finalDeviceParameters = _cor_setup->get_device_parameters();
        parametersGood = (_cor_setup->get_filter_converged() >= 1.) && !_cor_setup->get_failure_code() && !_cor_setup->sfm.calibration_bad;
    });
    if(parametersGood) [RCCalibration saveCalibrationData:finalDeviceParameters];
    return parametersGood;
}

- (void) captureMeasuredPhoto
{
}

- (void) receiveVideoFrame:(CMSampleBufferRef)sampleBuffer
{
    if(!isSensorFusionRunning) return;
    if(!CMSampleBufferDataIsReady(sampleBuffer) )
    {
        DLog( @"Sample buffer is not ready. Skipping sample." );
        return;
    }
    
    if (sampleBuffer) sampleBuffer = (CMSampleBufferRef)CFRetain(sampleBuffer);
    CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
    pixelBuffer = (CVPixelBufferRef)CVPixelBufferRetain(pixelBuffer);

    dispatch_async(inputQueue, ^{
        if (!isSensorFusionRunning) {
            CVPixelBufferRelease(pixelBuffer);
            CFRelease(sampleBuffer);
            return;
        }

        CMTime timestamp = (CMTime)CMSampleBufferGetPresentationTimeStamp(sampleBuffer);

        //capture image meta data
        //        CFDictionaryRef metadataDict = CMGetAttachment(sampleBuffer, kCGImagePropertyExifDictionary , NULL);
        //        DLog(@"metadata: %@", metadataDict);

        size_t width = CVPixelBufferGetWidth(pixelBuffer);
        size_t height = CVPixelBufferGetHeight(pixelBuffer);
        size_t stride = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0);
        if(width != 640 || height != 480 || stride != 640) {
            NSLog(@"Image dimensions are incorrect! Make sure you're using the right video preset and not changing the orientation on the capture connection.\n");
            abort();
        }
        uint64_t time_us = timestamp.value / (timestamp.timescale / 1000000.);

        CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
        unsigned char *pixel = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer,0);

        uint64_t offset_time = time_us + 16667;
        [self flushOperationsBeforeTime:offset_time];
        dispatch_async(queue, ^{
            if(filter_image_measurement(&_cor_setup->sfm, pixel, width, height, stride, offset_time)) {
                if(pixelBufferCached) {
                    CVPixelBufferUnlockBaseAddress(pixelBufferCached, kCVPixelBufferLock_ReadOnly);
                    CVPixelBufferRelease(pixelBufferCached);
                }
                pixelBufferCached = pixelBuffer;
                [self filterCallbackWithSampleBuffer:sampleBuffer];
                if (sampleBuffer) CFRelease(sampleBuffer);
            } else {
                CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
                CVPixelBufferRelease(pixelBuffer);
                if (sampleBuffer) CFRelease(sampleBuffer);
                [self filterCallbackWithSampleBuffer:nil];
            }
        });
    });
}

- (void) receiveAccelerometerData:(CMAccelerometerData *)accelerationData;
{
    if(!isSensorFusionRunning) return;
    dispatch_async(inputQueue, ^{
        if (!isSensorFusionRunning) return;
        uint64_t time = accelerationData.timestamp * 1000000;
        [self flushOperationsBeforeTime:time - 40000];
        [self enqueueOperation:[[RCSensorFusionOperation alloc] initWithBlock:^{
            float data[3];
            //ios gives acceleration in g-units, so multiply by standard gravity in m/s^2
            //it appears that accelerometer axes are flipped
            data[0] = -accelerationData.acceleration.x * 9.80665;
            data[1] = -accelerationData.acceleration.y * 9.80665;
            data[2] = -accelerationData.acceleration.z * 9.80665;
            filter_accelerometer_measurement(&_cor_setup->sfm, data, time);
            if(get_timestamp() - lastCallbackTime > minimumCallbackInterval)
                [self filterCallbackWithSampleBuffer:nil];
        } withTime:time]];
    });
}

- (void) receiveGyroData:(CMGyroData *)gyroData
{
    if(!isSensorFusionRunning) return;
    dispatch_async(inputQueue, ^{
        if (!isSensorFusionRunning) return;
        uint64_t time = gyroData.timestamp * 1000000;
        [self flushOperationsBeforeTime:time - 40000];
        [self enqueueOperation:[[RCSensorFusionOperation alloc] initWithBlock:^{
            float data[3];
            data[0] = gyroData.rotationRate.x;
            data[1] = gyroData.rotationRate.y;
            data[2] = gyroData.rotationRate.z;
            filter_gyroscope_measurement(&_cor_setup->sfm, data, time);
        } withTime:time]];
    });
}

/*
- (void) receiveMotionData:(CMDeviceMotion *)motionData
{
    if(!isSensorFusionRunning) return;
    if(isnan(motionData.gravity.x) || isnan(motionData.gravity.y) || isnan(motionData.gravity.z) || isnan(motionData.userAcceleration.x) || isnan(motionData.userAcceleration.y) || isnan(motionData.userAcceleration.z) || isnan(motionData.rotationRate.x) || isnan(motionData.rotationRate.y) || isnan(motionData.rotationRate.z)) return;
    dispatch_async(inputQueue, ^{
        if (!isSensorFusionRunning) return;
        uint64_t time = motionData.timestamp * 1000000;
        [self flushOperationsBeforeTime:time - 40000];
        [self enqueueOperation:[[RCSensorFusionOperation alloc] initWithBlock:^{
            float rot[3], grav[3];
            rot[0] = motionData.rotationRate.x;
            rot[1] = motionData.rotationRate.y;
            rot[2] = motionData.rotationRate.z;
            grav[0] = -motionData.gravity.x * 9.80665;
            grav[1] = -motionData.gravity.y * 9.80665;
            grav[2] = -motionData.gravity.z * 9.80665;
            filter_core_motion_measurement(&_cor_setup->sfm, rot, grav, time);
        } withTime:time]];
    });
}
*/

@end
