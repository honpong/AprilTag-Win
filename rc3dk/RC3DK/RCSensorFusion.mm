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

#ifdef LIBRARY
    #define SKIP_LICENSE_CHECK NO
#else
    #define SKIP_LICENSE_CHECK YES
#endif

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
    CVPixelBufferRef pixelBufferCached;
    dispatch_queue_t queue, inputQueue;
    NSMutableArray *dataWaiting;
    uint64_t lastCallbackTime;
    BOOL isLicenseValid;
    bool isStableStart;
}

#define minimumCallbackInterval 100000

- (void) validateLicense:(NSString*)apiKey withCompletionBlock:(void (^)(int, int))completionBlock withErrorBlock:(void (^)(NSError*))errorBlock
{
    if (SKIP_LICENSE_CHECK)
    {
        isLicenseValid = YES;
        if (completionBlock) completionBlock(RCLicenseTypeFull, RCLicenseStatusOK);
        return;
    }

    if (apiKey == nil || apiKey.length == 0)
    {
        if (errorBlock) errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorApiKeyMissing userInfo:@{NSLocalizedDescriptionKey: @"Failed to validate license. API key was nil or zero length.", NSLocalizedFailureReasonErrorKey: @"API key was nil or zero length."}]);
        return;
    }
    
    NSString* bundleId = [[NSBundle mainBundle] bundleIdentifier];
//    bundleId = @"com.realitycap.tapemeasure"; // for running unit tests only. getting bundle id doesn't work while running tests.
    if (bundleId == nil || bundleId.length == 0)
    {
        if (errorBlock) errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorBundleIdMissing userInfo:@{NSLocalizedDescriptionKey: @"Failed to validate license. Could not get bundle ID.", NSLocalizedFailureReasonErrorKey: @"Could not get bundle ID."}]);
        return;
    }

    NSString* vendorId = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
    if (vendorId == nil || vendorId.length == 0)
    {
        if (errorBlock) errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorVendorIdMissing userInfo:@{NSLocalizedDescriptionKey: @"Failed to validate license. Could not get ID for vendor.", NSLocalizedFailureReasonErrorKey: @"Could not get ID for vendor."}]);
        return;
    }

    NSDictionary *params = @{@"requested_resource": @"3DK_TR",
                            @"api_key": apiKey,
                            @"bundle_id": bundleId,
                            @"vendor_id": vendorId};
    
    RCPrivateHTTPClient*client = [RCPrivateHTTPClient sharedInstance];
    
    [client
     postPath:API_LICENSING_POST
     parameters:params
     success:^(RCAFHTTPRequestOperation *operation, id JSON)
     {
         DLog(@"License completion %li\n%@", (long)operation.response.statusCode, operation.responseString);
         if (operation.response.statusCode != 200)
         {
             if (errorBlock) errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorHttpError userInfo:@{NSLocalizedDescriptionKey: [NSString stringWithFormat:@"Failed to validate license. HTTP response code %li.", (long)operation.response.statusCode], NSLocalizedFailureReasonErrorKey: [NSString stringWithFormat:@"HTTP status %li: %@", (long)operation.response.statusCode, operation.responseString]}]);
             return;
         }
         
         if (JSON == nil)
         {
             if (errorBlock) errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorEmptyResponse userInfo:@{NSLocalizedDescriptionKey: @"Failed to validate license. Response body was empty.", NSLocalizedFailureReasonErrorKey: @"Response body was empty."}]);
             return;
         }
         
         NSError* serializationError;
         NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:&serializationError];
         if (serializationError || response == nil)
         {
             if (errorBlock)
             {
                 NSMutableDictionary* userInfo = [NSMutableDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. Failed to deserialize response.", NSLocalizedDescriptionKey, @"Failed to deserialize response.", NSLocalizedFailureReasonErrorKey, nil];
                 if (serializationError) userInfo[NSUnderlyingErrorKey] = serializationError;
                 errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorDeserialization userInfo:userInfo]);
             }
             return;
         }
         
         NSNumber* licenseStatus = response[@"license_status"];
         NSNumber* licenseType = response[@"license_type"];
         
         if (licenseStatus == nil || licenseType == nil)
         {
             if (errorBlock) errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorInvalidResponse userInfo:@{NSLocalizedDescriptionKey: @"Failed to validate license. Invalid response from server.", NSLocalizedFailureReasonErrorKey: @"Invalid response from server."}]);
             return;
         }
         
         if ([licenseStatus intValue] == RCLicenseStatusOK) isLicenseValid = YES; // TODO: handle other license types
         
         if (completionBlock) completionBlock([licenseType intValue], [licenseStatus intValue]);
     }
     failure:^(RCAFHTTPRequestOperation *operation, NSError *error)
     {
         DLog(@"License failure: %li\n%@", (long)operation.response.statusCode, operation.responseString);
         if (errorBlock)
         {
             NSMutableDictionary* userInfo = [NSMutableDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. HTTPS request failed.", NSLocalizedDescriptionKey, @"HTTPS request failed. See underlying error.", NSLocalizedFailureReasonErrorKey, nil];
             if (error) userInfo[NSUnderlyingErrorKey] = error;
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
        dataWaiting = [NSMutableArray arrayWithCapacity:10];
        queue = dispatch_queue_create("com.realitycap.sensorfusion", DISPATCH_QUEUE_SERIAL);
        inputQueue = dispatch_queue_create("com.realitycap.sensorfusion.input", DISPATCH_QUEUE_SERIAL);
        lastCallbackTime = 0;
        
        [RCPrivateHTTPClient initWithBaseUrl:API_BASE_URL withAcceptHeader:API_HEADER_ACCEPT withApiVersion:API_VERSION];
        
        dispatch_async(queue, ^{
            corvis_device_parameters dc = [RCCalibration getCalibrationData];
            _cor_setup = new filter_setup(&dc);
            cor_time_init();
            plugins_start();
        });
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

    dispatch_sync(queue, ^{
        plugins_stop();
        if(_cor_setup) delete _cor_setup;
        plugins_clear();
    });
}

- (void) startReplay
{
    _cor_setup->sfm.ignore_lateness = true;
}

- (void) startInertialOnlyFusion __attribute((deprecated("No longer needed; does nothing.")))
{
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
    if(isSensorFusionRunning) return;
    dispatch_async(queue, ^{
        filter_start_static_calibration(&_cor_setup->sfm);
    });
    isSensorFusionRunning = true;
}

- (void) stopStaticCalibration __attribute((deprecated("Use stopSensorFusion instead.")))
{
    [self stopSensorFusion];
}

- (void) focusOperationFinished:(bool)timedOut
{
    // startProcessingVideo
    if(processingVideoRequested && !isProcessingVideo) {
        isProcessingVideo = true;
        processingVideoRequested = false;
    }
}

- (void) startProcessingVideoWithDevice:(AVCaptureDevice *)device __attribute((deprecated("Use startSensorFusionWithDevice instead.")))
{
    [self startSensorFusionWithDevice:device];
}

- (void) startSensorFusionWithDevice:(AVCaptureDevice *)device
{
    if(isProcessingVideo || processingVideoRequested || isSensorFusionRunning) return;
    isStableStart = true;
    if (SKIP_LICENSE_CHECK || isLicenseValid)
    {
        isLicenseValid = NO; // evaluation license must be checked every time. need more logic here for other license types.
        dispatch_async(queue, ^{
            filter_start_hold_steady(&_cor_setup->sfm);
        });
        RCCameraManager * cameraManager = [RCCameraManager sharedInstance];

        isProcessingVideo = false;
        processingVideoRequested = true;
        cameraManager.delegate = self;
        [cameraManager setVideoDevice:device];
        [cameraManager lockFocus];
        isSensorFusionRunning = true;
    }
    else if ([self.delegate respondsToSelector:@selector(sensorFusionError:)])
    {
        NSDictionary* userInfo = @{NSLocalizedDescriptionKey: @"Cannot start sensor fusion. License needs to be validated."};
        NSError *error =[[NSError alloc] initWithDomain:ERROR_DOMAIN code:RCSensorFusionErrorCodeLicense userInfo:userInfo];
        [self.delegate sensorFusionError:error];
    }
}

- (void) startSensorFusionUnstableWithDevice:(AVCaptureDevice *)device
{
    if(isProcessingVideo || processingVideoRequested || isSensorFusionRunning) return;
    isStableStart = false;
    if (SKIP_LICENSE_CHECK || isLicenseValid)
    {
        isLicenseValid = NO; // evaluation license must be checked every time. need more logic here for other license types.
        dispatch_async(queue, ^{
            filter_start_dynamic(&_cor_setup->sfm);
        });
        RCCameraManager * cameraManager = [RCCameraManager sharedInstance];
        
        isProcessingVideo = false;
        processingVideoRequested = true;
        isSensorFusionRunning = true;
        cameraManager.delegate = self;
        [cameraManager setVideoDevice:device];
        [cameraManager lockFocus];
    }
    else if ([self.delegate respondsToSelector:@selector(sensorFusionError:)])
    {
        NSDictionary* userInfo = @{NSLocalizedDescriptionKey: @"Cannot start sensor fusion. License needs to be validated."};
        NSError *error =[[NSError alloc] initWithDomain:ERROR_DOMAIN code:RCSensorFusionErrorCodeLicense userInfo:userInfo];
        [self.delegate sensorFusionError:error];
    }
}

- (void) stopProcessingVideo __attribute((deprecated("Use stopSensorFusion instead.")))
{
    [self stopSensorFusion];
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

    [dataWaiting removeAllObjects];
    dispatch_sync(inputQueue, ^{
        isSensorFusionRunning = false;
        isProcessingVideo = false;
        processingVideoRequested = false;
        
        [self saveCalibration];
        dispatch_sync(queue, ^{
            filter_initialize(&_cor_setup->sfm, _cor_setup->device);
        });
    });

    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        [RCCalibration postDeviceCalibration:nil onFailure:nil];
    });
    
    RCCameraManager * cameraManager = [RCCameraManager sharedInstance];
    [cameraManager releaseVideoDevice];

}

- (void) filterCallbackWithSampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    if (sampleBuffer) sampleBuffer = (CMSampleBufferRef)CFRetain(sampleBuffer);
    
    //handle errors first, so that we don't provide invalid data. get error codes before handle_errors so that we don't reset them without reading first
    int failureCode = _cor_setup->get_failure_code();
    RCSensorFusionErrorCode errorCode = _cor_setup->get_error();

    //perform these operations synchronously in the calling (filter) thread
    struct filter *f = &(_cor_setup->sfm);
    float converged = _cor_setup->get_filter_converged();

    bool steady = _cor_setup->get_device_steady();
    
        
    RCSensorFusionStatus* status = [[RCSensorFusionStatus alloc] initWithState:f->SensorFusionState withProgress:converged withErrorCode:failureCode withIsSteady:steady];
    RCTranslation* translation = [[RCTranslation alloc] initWithVector:f->s.T.v withStandardDeviation:v4_sqrt(f->s.T.variance())];
    RCRotation* rotation = [[RCRotation alloc] initWithVector:f->s.W.v.raw_vector() withStandardDeviation:v4_sqrt(v4(f->s.W.variance()))];
    RCTransformation* transformation = [[RCTransformation alloc] initWithTranslation:translation withRotation:rotation];

    RCTranslation* camT = [[RCTranslation alloc] initWithVector:f->s.Tc.v withStandardDeviation:v4_sqrt(f->s.Tc.variance())];
    RCRotation* camR = [[RCRotation alloc] initWithVector:f->s.Wc.v.raw_vector() withStandardDeviation:v4_sqrt(v4(f->s.Wc.variance()))];
    RCTransformation* camTransform = [[RCTransformation alloc] initWithTranslation:camT withRotation:camR];
    
    RCScalar *totalPath = [[RCScalar alloc] initWithScalar:f->s.total_distance withStdDev:0.];
    
    RCCameraParameters *camParams = [[RCCameraParameters alloc] initWithFocalLength:f->s.focal_length.v withOpticalCenterX:f->s.center_x.v withOpticalCenterY:f->s.center_y.v withRadialSecondDegree:f->s.k1.v withRadialFourthDegree:f->s.k2.v];

    RCSensorFusionData* data = [[RCSensorFusionData alloc] initWithStatus:status withTransformation:transformation withCameraTransformation:[transformation composeWithTransformation:camTransform] withCameraParameters:camParams withTotalPath:totalPath withFeatures:[self getFeaturesArray] withSampleBuffer:sampleBuffer withTimestamp:f->last_time];

    // queue actions related to failures before queuing callbacks to the sdk client.
    if(errorCode == RCSensorFusionErrorCodeTooFast || errorCode == RCSensorFusionErrorCodeOther) {
        isProcessingVideo = false;
        processingVideoRequested = false;
        isSensorFusionRunning = false;
        dispatch_async(dispatch_get_main_queue(), ^{
            [dataWaiting removeAllObjects];
            RCCameraManager * cameraManager = [RCCameraManager sharedInstance];
            [cameraManager releaseVideoDevice];
        });
    }

    if(errorCode == RCSensorFusionErrorCodeVision && (f->SensorFusionState != RCSensorFusionStateRunning)) {
        isProcessingVideo = false;
        processingVideoRequested = true;

        // Request a refocus
        RCCameraManager * cameraManager = [RCCameraManager sharedInstance];
        [cameraManager focusOnceAndLock];
    }

    //send the callback to the main/ui thread
    dispatch_async(dispatch_get_main_queue(), ^{
        if ([self.delegate respondsToSelector:@selector(sensorFusionDidUpdate:)]) [self.delegate sensorFusionDidUpdate:data];
        if (errorCode)
        {
            if([self.delegate respondsToSelector:@selector(sensorFusionError:)]) {
                NSError *error =[[NSError alloc] initWithDomain:ERROR_DOMAIN code:errorCode userInfo:nil];
                [self.delegate sensorFusionError:error];
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
        if(i->is_valid()) {
            f_t stdev = i->v.stdev_meters(sqrt(i->variance()));
            
            RCFeaturePoint* feature = [[RCFeaturePoint alloc] initWithId:i->id withX:i->current[0] withY:i->current[1] withOriginalDepth:[[RCScalar alloc] initWithScalar:i->v.depth() withStdDev:stdev] withWorldPoint:[[RCPoint alloc]initWithX:i->world[0] withY:i->world[1] withZ:i->world[2]] withInitialized:i->is_initialized()];
            [array addObject:feature];
        }
    }
    return [NSArray arrayWithArray:array];
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
        parametersGood = !_cor_setup->get_failure_code() && !_cor_setup->sfm.calibration_bad;
    });
    if(parametersGood) [RCCalibration saveCalibrationData:finalDeviceParameters];
    return parametersGood;
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
        bool isPlanar = CVPixelBufferIsPlanar(pixelBuffer);
        size_t stride;
        if(isPlanar)
            stride = CVPixelBufferGetBytesPerRowOfPlane(pixelBuffer, 0);
        else
            stride = CVPixelBufferGetBytesPerRow(pixelBuffer);

        if(width != 640 || height != 480 || stride != 640) {
            NSLog(@"Image dimensions are incorrect! Make sure you're using the right video preset and not changing the orientation on the capture connection.\n");
            abort();
        }
        uint64_t time_us = timestamp.value / (timestamp.timescale / 1000000.);

        CVPixelBufferLockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
        unsigned char * pixel;
        if(isPlanar)
            pixel = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer,0);
        else
            pixel = (unsigned char *)CVPixelBufferGetBaseAddress(pixelBuffer);

        uint64_t offset_time = time_us + 16667;
        //TODO: sometimes we are getting packets out of order, so add a 10 ms delay to dispatch of video frames to make sure we get next accel/gyro frame. tricky because we need to clean up if it gets stuck in the queue, which we don't need to do for accel/gyro
        [self flushOperationsBeforeTime:offset_time];
        dispatch_async(queue, ^{
            bool docallback = true;
            if(isProcessingVideo) {
                docallback = filter_image_measurement(&_cor_setup->sfm, pixel, (int)width, (int)height, (int)stride, offset_time);
            }
            if(docallback) {
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
