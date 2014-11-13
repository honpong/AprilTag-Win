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

#pragma mark - QR Code detection stuff for ViewAR

@interface QRDetection : NSObject

@property NSString * code;
@property NSArray * corners;
@property uint64_t time;

@property RCTransformation* transformation;

@end

@implementation QRDetection

@end

@interface QRSyncData : NSObject

@property NSMutableArray * savedTransformations;
@property NSMutableArray * savedTimes;
@property NSMutableArray * detections;
@property RCCameraParameters * camera;
@property size_t imageWidth, imageHeight;

@end

@implementation QRSyncData
@synthesize savedTransformations, savedTimes, detections, camera, imageWidth, imageHeight;

-(id)init
{
    if (self = [super init])
    {
        savedTransformations = [NSMutableArray array];
        savedTimes = [NSMutableArray array];
        detections = [NSMutableArray array];
    }
    return self;
}

- (void) handleSyncedDetection:(QRDetection *)detection
{
    CGPoint topleft, bottomleft, bottomright, topright;
    CGPointMakeWithDictionaryRepresentation((CFDictionaryRef)[detection.corners objectAtIndex:0], &topleft);
    CGPointMakeWithDictionaryRepresentation((CFDictionaryRef)[detection.corners objectAtIndex:1], &bottomleft);
    CGPointMakeWithDictionaryRepresentation((CFDictionaryRef)[detection.corners objectAtIndex:2], &bottomright);
    CGPointMakeWithDictionaryRepresentation((CFDictionaryRef)[detection.corners objectAtIndex:3], &topright);
    
    NSLog(@"Code detected %@ at %llu, corners (%f, %f), (%f, %f), (%f, %f), (%f, %f)", detection.code, detection.time, topleft.x * imageWidth, topleft.y * imageHeight, bottomleft.x * imageWidth, bottomleft.y * imageHeight, bottomright.x * imageWidth, bottomright.y * imageHeight, topright.x * imageWidth, topright.y * imageHeight);
    
    
    /*
     What needs to be done. Derivation follows: http://vision.ucla.edu//MASKS/MASKS-ch5.pdf section 5.3
     
     X1 = 3D position of corner in QR-centered coordinates
     R,T = transformation bringing camera coordinates to QR-centered coordinates
     p = unknown projective scale factor (z2)
     X2 = (x2, y2, 1) - normalized image coordinates of projected point
     
     X1 = R p X2 + T
     
     N = normal to plane where the 4 features lie (in camera coordinates)
     d = distance from camera to the plane (not to the point)
     N^t p X2 = d follows from above two definitions, so
     1/d N^t p X2 = 1
     
     Now we multiply T by 1, and subtitute in the above.
     
     X1 = R p X2 + T / d N^t p X2
     X1 = H p X2, where H = R + T / d N^t
     
     Take cross product, X1 x X1 = 0; express as X1^ X1, where X1^ is (X1 hat) the skew-symmetric matrix for cross product.
     
     X1^ = [0   -z1  y1]
     [x1    0 -x1]
     [-y1  x1   0]
     
     X1^ X1 = X1^ H p X2
     0 = X1^ H p X2; p is a non-zero scalar, so we can divide it out
     0 = X1^ H X2
     
     Now work out the coefficients
     
     H X2 = [ H0 H1 H2 ] [x2]   [H0x2+H1y2+H2]
     [ H3 H4 H5 ]*[y2] = [H3x2+H4y2+H5]
     [ H6 H7 H8 ] [ 1]   [H6x2+H7y2+H8]
     
     [0       0       0       -z1x2   -z1y2   -z1     y1x2    y1y2    y  ]   [H0]
     X1^ H X2 = [z1x2    z1y2    z1      0       0       0       -x1x2   -x1y2   -x1] * [H1]
     [-y1x2   -y1y2   -y1     x1x2    x1y2    x1      0       0       0  ]   [H2]
     [H3]
     [H4]
     [H5]
     [H6]
     [H7]
     [H8]
     
     The X1X2 matrix only has rank 2, so we just use the first two constraints for each pixel.
     
     Decomposition of planar homography matrix should follow Masks derivation.
     */
    
    
}

- (void) syncDetections
{
    if(![savedTimes count] || ![detections count]) return;
    
    NSMutableArray * emitObjects = [NSMutableArray array];
    NSMutableArray * deleteObjects = [NSMutableArray array];
    for(QRDetection * detection in detections)
    {
        NSNumber * time = [NSNumber numberWithUnsignedLongLong:detection.time];
        if(time > [savedTimes lastObject]) // haven't received sensor fusion data yet
            continue;
        
        if(time < [savedTimes firstObject]) {
            NSLog(@"Warning: dropping a detection, didn't associate it in time");
            [deleteObjects addObject:detection];
            continue;
        }
        
        NSRange searchRange = NSMakeRange(0, [savedTimes count]);
        NSUInteger findIndex = [savedTimes indexOfObject:time
                                           inSortedRange:searchRange
                                                 options:NSBinarySearchingFirstEqual
                                         usingComparator:^(id obj1, id obj2)
                                {
                                    return [obj1 compare:obj2];
                                }];
        
        if(findIndex != NSNotFound) {
            detection.transformation = [savedTransformations objectAtIndex:findIndex];
            [emitObjects addObject:detection];
        }
    }
    
    for(QRDetection * detection in emitObjects) {
        [self handleSyncedDetection:detection];
    }

    [detections removeObjectsInArray:emitObjects];
    [detections removeObjectsInArray:deleteObjects];
}

- (BOOL) detectionIsLate:(uint64_t)timestamp {
    if ([savedTimes count]) {
        NSNumber * startNum = [savedTimes objectAtIndex:0];
        uint64_t start = [startNum unsignedLongLongValue];
        if(timestamp < start)
            return true;
    }
    return false;
}

- (void) addSensorFusionData:(RCSensorFusionData *)data
{
    @synchronized(savedTimes) {
        camera = data.cameraParameters;
        CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(data.sampleBuffer);
        
        imageWidth = CVPixelBufferGetWidth(pixelBuffer);
        imageHeight = CVPixelBufferGetHeight(pixelBuffer);
        
        CMTime time = CMSampleBufferGetPresentationTimeStamp(data.sampleBuffer);
        uint64_t time_us = time.value / (time.timescale / 1000000.);
        [savedTransformations addObject:data.cameraTransformation];
        [savedTimes addObject:[NSNumber numberWithUnsignedLongLong:time_us]];
        if([savedTransformations count] > 100)
            [savedTransformations removeObjectAtIndex:0];
        if([savedTimes count] > 100)
            [savedTimes removeObjectAtIndex:0];
        
        [self syncDetections];
    }
}

- (void) observeQR:(AVMetadataMachineReadableCodeObject *)QRData
{
    @synchronized(savedTimes) {
        uint64_t timestamp = QRData.time.value / (QRData.time.timescale / 1000000.);
        
        if([self detectionIsLate:timestamp]) {
            NSLog(@"Warning: Detection came too late");
            return;
        }
        
        QRDetection * detection = [[QRDetection alloc] init];
        detection.code = QRData.stringValue;
        detection.time = timestamp;
        detection.corners = QRData.corners;
        [detections addObject:detection];
        
        [self syncDetections];
    }
}

@end

#pragma mark -

@interface RCSensorFusion () <RCCameraManagerDelegate>

@property (nonatomic) RCLicenseType licenseType;

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
    BOOL isLicenseValid;
    bool isStableStart;
    RCSensorFusionRunState lastRunState;
    RCSensorFusionErrorCode lastErrorCode;
    float lastProgress;
    NSString* licenseKey;
    QRSyncData *QRSync;
}

- (void) setLicenseKey:(NSString*)licenseKey_
{
    licenseKey = licenseKey_;
}

- (void) validateLicense:(NSString*)apiKey withCompletionBlock:(void (^)(int licenseType, int licenseStatus))completionBlock withErrorBlock:(void (^)(NSError*))errorBlock
{
    if (SKIP_LICENSE_CHECK)
    {
        isLicenseValid = YES;
        if (completionBlock) completionBlock(RCLicenseTypeFull, RCLicenseStatusOK);
        return;
    }

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
    
    NSString* bundleId = [[NSBundle mainBundle] bundleIdentifier];
//    bundleId = @"com.realitycap.tapemeasure"; // for running unit tests only. getting bundle id doesn't work while running tests.
    if (bundleId == nil || bundleId.length == 0)
    {
        if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorBundleIdMissing userInfo:@{NSLocalizedDescriptionKey: @"Failed to validate license. Could not get bundle ID.", NSLocalizedFailureReasonErrorKey: @"Could not get bundle ID."}]);
        return;
    }

    NSString* vendorId = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
    if (vendorId == nil || vendorId.length == 0)
    {
        if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorVendorIdMissing userInfo:@{NSLocalizedDescriptionKey: @"Failed to validate license. Could not get ID for vendor.", NSLocalizedFailureReasonErrorKey: @"Could not get ID for vendor."}]);
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
             if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorHttpError userInfo:@{NSLocalizedDescriptionKey: [NSString stringWithFormat:@"Failed to validate license. HTTP response code %li.", (long)operation.response.statusCode], NSLocalizedFailureReasonErrorKey: [NSString stringWithFormat:@"HTTP status %li: %@", (long)operation.response.statusCode, operation.responseString]}]);
             return;
         }
         
         if (JSON == nil)
         {
             if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorEmptyResponse userInfo:@{NSLocalizedDescriptionKey: @"Failed to validate license. Response body was empty.", NSLocalizedFailureReasonErrorKey: @"Response body was empty."}]);
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
                 errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorDeserialization userInfo:userInfo]);
             }
             return;
         }
         
         NSNumber* licenseStatusString = response[@"license_status"];
         NSNumber* licenseTypeString = response[@"license_type"];
         
         if (licenseStatusString == nil || licenseTypeString == nil)
         {
             if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorInvalidResponse userInfo:@{NSLocalizedDescriptionKey: @"Failed to validate license. Invalid response from server.", NSLocalizedFailureReasonErrorKey: @"Invalid response from server."}]);
             return;
         }
         
         int licenseStatus = [licenseStatusString intValue];
         int licenseType = [licenseTypeString intValue];
         
         switch (licenseStatus)
         {
             case RCLicenseStatusOK:
                 isLicenseValid = YES;
                 if (completionBlock) completionBlock(licenseType, licenseStatus);
                 break;
                 
             case RCLicenseStatusOverLimit:
                 if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorOverLimit userInfo:nil]);
                 break;
                 
             case RCLicenseStatusRateLimited:
                 if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorRateLimited userInfo:nil]);
                 break;
                 
             case RCLicenseStatusSuspended:
                 if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorSuspended userInfo:nil]);
                 break;
                 
             case RCLicenseStatusInvalid:
                 if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorInvalidKey userInfo:nil]);
                 break;
                 
             default:
                 if (errorBlock) errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorUnknown userInfo:nil]);
                 break;
         }
     }
     failure:^(RCAFHTTPRequestOperation *operation, NSError *error)
     {
         DLog(@"License failure: %li\n%@", (long)operation.response.statusCode, operation.responseString);
         if (errorBlock)
         {
             NSMutableDictionary* userInfo = [NSMutableDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. HTTPS request failed.", NSLocalizedDescriptionKey, @"HTTPS request failed. See underlying error.", NSLocalizedFailureReasonErrorKey, nil];
             if (error) userInfo[NSUnderlyingErrorKey] = error;
             errorBlock([RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCLicenseErrorHttpFailure userInfo:userInfo]);
         }
     }
     ];
}

- (void) validateLicenseInternal
{
    [self validateLicense:licenseKey withCompletionBlock:^(int licenseType, int licenseStatus) {
        // all good. do nothing.
    } withErrorBlock:^(NSError* error) {
        [self handleLicenseError:error];
    }];
}

- (void) handleLicenseError:(NSError*)error
{
    if (isSensorFusionRunning)
    {
        [self stopSensorFusion];
        
        if ([self.delegate respondsToSelector:@selector(sensorFusionDidChangeStatus:)])
        {
            RCSensorFusionStatus* status = [[RCSensorFusionStatus alloc] initWithRunState:RCSensorFusionRunStateInactive withProgress:0 withError:error];
            [self.delegate sensorFusionDidChangeStatus:status];
        }
    }
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
        if(time && op.time >= time) break;
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
        lastRunState = RCSensorFusionRunStateInactive;
        lastErrorCode = RCSensorFusionErrorCodeNone;
        lastProgress = 0.;
        licenseKey = nil;
        
        QRSync = [[QRSyncData alloc] init];
        
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
    dispatch_sync(queue, ^{
        _cor_setup->sfm.ignore_lateness = true;
    });
}

- (void) startInertialOnlyFusion __attribute((deprecated("No longer needed; does nothing.")))
{
}

- (void) setLocation:(CLLocation*)location
{
    if(location)
    {
        dispatch_async(queue, ^{ filter_compute_gravity(&_cor_setup->sfm, location.coordinate.latitude, location.altitude); } );
        DLog(@"Sensor fusion location set: %@", location);
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
        [RCCalibration clearCalibrationData];
        _cor_setup->device = [RCCalibration getCalibrationData];
        filter_initialize(&_cor_setup->sfm, _cor_setup->device);
        filter_start_static_calibration(&_cor_setup->sfm);
    });
    isSensorFusionRunning = true;
    
    [self validateLicenseInternal];
}

- (void) stopStaticCalibration __attribute((deprecated("No longer needed; does nothing.")))
{
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
    
    [self validateLicenseInternal];
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
    else if ([self.delegate respondsToSelector:@selector(sensorFusionDidChangeStatus:)])
    {
        RCLicenseError* error = [RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCSensorFusionErrorCodeLicense userInfo:nil]; // TODO: use real error code
        RCSensorFusionStatus *status = [[RCSensorFusionStatus alloc] initWithRunState:_cor_setup->sfm.run_state withProgress:_cor_setup->get_filter_converged() withError:error];
        [self.delegate sensorFusionDidChangeStatus:status];
    }
}

- (void) stopProcessingVideo __attribute((deprecated("Use stopSensorFusion instead.")))
{
    [self stopSensorFusion];
}

/* Documentation in case we ever get around to implementing this
 
 Request that sensor fusion attempt to track a user-selected feature.
 
 If you call this method, the sensor fusion algorithm will make its best effort to detect and track a visual feature near the specified image coordinates. There is no guarantee that such a feature may be identified or tracked for any length of time (for example, if you specify coordinates in the middle of a blank wall, no feature will be found. Any such feature is also not likely to be found at the exact pixel coordinates specified.
 @param x The requested horizontal location, in pixels relative to the image coordinate frame.
 @param x The requested vertical location, in pixels relative to the image coordinate frame.
 */
//- (void) selectUserFeatureWithX:(float)x withY:(float)Y;
- (void) selectUserFeatureWithX:(float)x withY:(float)y
{
    if(!isProcessingVideo) return;
    dispatch_async(queue, ^{ filter_select_feature(&_cor_setup->sfm, x, y); });
}

- (void) flushAndReset
{
    isSensorFusionRunning = false;
    dispatch_sync(inputQueue, ^{
        [self flushOperationsBeforeTime:0];
    });
    dispatch_sync(queue, ^{
        filter_initialize(&_cor_setup->sfm, _cor_setup->device);
        if(pixelBufferCached) {
            CVPixelBufferUnlockBaseAddress(pixelBufferCached, kCVPixelBufferLock_ReadOnly);
            CVPixelBufferRelease(pixelBufferCached);
            pixelBufferCached = nil;
        }
    });
    RCCameraManager * cameraManager = [RCCameraManager sharedInstance];
    [cameraManager releaseVideoDevice];
    isProcessingVideo = false;
    processingVideoRequested = false;
}

- (void) stopSensorFusion
{
    LOGME
    if(!isSensorFusionRunning) return;
    [self saveCalibration];
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
        [RCCalibration postDeviceCalibration:nil onFailure:nil];
    });
    [self flushAndReset];
}

- (void) sendStatus
{
    //perform these operations synchronously in the calling (filter) thread
    struct filter *f = &(_cor_setup->sfm);
    
    //handle errors first, so that we don't provide invalid data. get error codes before handle_errors so that we don't reset them without reading first
    RCSensorFusionErrorCode errorCode = _cor_setup->get_error();
    float converged = _cor_setup->get_filter_converged();
    
    if((converged == lastProgress) && (errorCode == lastErrorCode) && (f->run_state == lastRunState)) return;

    // queue actions related to failures before queuing callbacks to the sdk client.
    if(errorCode == RCSensorFusionErrorCodeTooFast || errorCode == RCSensorFusionErrorCodeOther)
    {
        //Sensor fusion has already been reset by get_error, but it could have gotten random data inbetween, so do full reset
        dispatch_async(dispatch_get_main_queue(), ^{
            [self flushAndReset];
        });
    } else if(lastRunState == RCSensorFusionRunStateStaticCalibration && f->run_state == RCSensorFusionRunStateInactive && errorCode == RCSensorFusionErrorCodeNone)
    {
        isSensorFusionRunning = false;
        dispatch_async(dispatch_get_main_queue(), ^{
            [self stopSensorFusion];
        });
    }
    
    if((errorCode == RCSensorFusionErrorCodeVision && f->run_state != RCSensorFusionRunStateRunning) || (f->run_state == RCSensorFusionRunStateSteadyInitialization && converged < .1)) {
        //refocus if either we tried to detect and failed, or if we've recently moved during initialization
        isProcessingVideo = false;
        processingVideoRequested = true;
        
        // Request a refocus
        RCCameraManager * cameraManager = [RCCameraManager sharedInstance];
        [cameraManager focusOnceAndLock];
    }
    
    if((converged != lastProgress) || (errorCode != lastErrorCode) || (f->run_state != lastRunState))
    {
        RCSensorFusionError* error = nil;
        if (errorCode != RCSensorFusionErrorCodeNone) error = [RCSensorFusionError errorWithDomain:ERROR_DOMAIN code:errorCode userInfo:nil];
        
        RCSensorFusionStatus* status = [[RCSensorFusionStatus alloc] initWithRunState:f->run_state withProgress:converged withError:error];
        dispatch_async(dispatch_get_main_queue(), ^{
            if ([self.delegate respondsToSelector:@selector(sensorFusionDidChangeStatus:)]) [self.delegate sensorFusionDidChangeStatus:status];
        });
    }
    lastErrorCode = errorCode;
    lastRunState = f->run_state;
    lastProgress = converged;
}

- (void) sendDataWithSampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    //perform these operations synchronously in the calling (filter) thread
    struct filter *f = &(_cor_setup->sfm);
    
    if (sampleBuffer) sampleBuffer = (CMSampleBufferRef)CFRetain(sampleBuffer);
    RCTranslation* translation = [[RCTranslation alloc] initWithVector:f->s.T.v withStandardDeviation:v4_sqrt(f->s.T.variance())];
    RCRotation* rotation = [[RCRotation alloc] initWithVector:f->s.W.v.raw_vector() withStandardDeviation:v4_sqrt(v4(f->s.W.variance()))];
    RCTransformation* transformation = [[RCTransformation alloc] initWithTranslation:translation withRotation:rotation];

    RCTranslation* camT = [[RCTranslation alloc] initWithVector:f->s.Tc.v withStandardDeviation:v4_sqrt(f->s.Tc.variance())];
    RCRotation* camR = [[RCRotation alloc] initWithVector:f->s.Wc.v.raw_vector() withStandardDeviation:v4_sqrt(v4(f->s.Wc.variance()))];
    RCTransformation* camTransform = [[RCTransformation alloc] initWithTranslation:camT withRotation:camR];
    
    RCScalar *totalPath = [[RCScalar alloc] initWithScalar:f->s.total_distance withStdDev:0.];
    
    RCCameraParameters *camParams = [[RCCameraParameters alloc] initWithFocalLength:f->s.focal_length.v withOpticalCenterX:f->s.center_x.v withOpticalCenterY:f->s.center_y.v withRadialSecondDegree:f->s.k1.v withRadialFourthDegree:f->s.k2.v];

    RCSensorFusionData* data = [[RCSensorFusionData alloc] initWithTransformation:transformation withCameraTransformation:[transformation composeWithTransformation:camTransform] withCameraParameters:camParams withTotalPath:totalPath withFeatures:[self getFeaturesArray] withSampleBuffer:sampleBuffer withTimestamp:f->last_time];

    [QRSync addSensorFusionData:data];
    //send the callback to the main/ui thread
    dispatch_async(dispatch_get_main_queue(), ^{
        if ([self.delegate respondsToSelector:@selector(sensorFusionDidUpdateData:)]) [self.delegate sensorFusionDidUpdateData:data];
        if(sampleBuffer) CFRelease(sampleBuffer);
    });
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
    CMTime timestamp = (CMTime)CMSampleBufferGetPresentationTimeStamp(sampleBuffer);
    uint64_t time_us = timestamp.value / (timestamp.timescale / 1000000.);
    uint64_t now = get_timestamp();
    if(now - time_us > 100000)
    {
        DLog(@"Warning, got an old video frame - timestamp %lld, now %lld\n", time_us, now);
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
            if(!isSensorFusionRunning)
            {
                CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
                CVPixelBufferRelease(pixelBuffer);
                if (sampleBuffer) CFRelease(sampleBuffer);
                return;
            } else if(isProcessingVideo) {
                docallback = filter_image_measurement(&_cor_setup->sfm, pixel, (int)width, (int)height, (int)stride, offset_time);
            } else {
                //We're not actually running, but we do want to send updates for the video preview. Make sure that rotation is initialized.
                docallback =  _cor_setup->sfm.gravity_init;
            }
            if(docallback) {
                if(pixelBufferCached) {
                    CVPixelBufferUnlockBaseAddress(pixelBufferCached, kCVPixelBufferLock_ReadOnly);
                    CVPixelBufferRelease(pixelBufferCached);
                }
                pixelBufferCached = pixelBuffer;
                [self sendStatus];
                [self sendDataWithSampleBuffer:sampleBuffer];
                if (sampleBuffer) CFRelease(sampleBuffer);
            } else {
                CVPixelBufferUnlockBaseAddress(pixelBuffer, kCVPixelBufferLock_ReadOnly);
                CVPixelBufferRelease(pixelBuffer);
                if (sampleBuffer) CFRelease(sampleBuffer);
                [self sendStatus];
            }
        });
    });
}

- (void) receiveAccelerometerData:(CMAccelerometerData *)accelerationData;
{
    if(!isSensorFusionRunning) return;
    uint64_t time_us = accelerationData.timestamp * 1000000;
    uint64_t now = get_timestamp();
    if(now - time_us > 40000)
    {
        DLog(@"Warning, got an old accelerometer sample - timestamp %lld, now %lld\n", time_us, now);
        return;
    }
    dispatch_async(inputQueue, ^{
        if (!isSensorFusionRunning) return;
        uint64_t time = accelerationData.timestamp * 1000000;
        [self flushOperationsBeforeTime:time - 40000];
        [self enqueueOperation:[[RCSensorFusionOperation alloc] initWithBlock:^{
            if(!isSensorFusionRunning) return;
            float data[3];
            //ios gives acceleration in g-units, so multiply by standard gravity in m/s^2
            //it appears that accelerometer axes are flipped
            data[0] = -accelerationData.acceleration.x * 9.80665;
            data[1] = -accelerationData.acceleration.y * 9.80665;
            data[2] = -accelerationData.acceleration.z * 9.80665;
            filter_accelerometer_measurement(&_cor_setup->sfm, data, time);
            [self sendStatus];
        } withTime:time]];
    });
}

- (void) receiveGyroData:(CMGyroData *)gyroData
{
    if(!isSensorFusionRunning) return;
    uint64_t time_us = gyroData.timestamp * 1000000;
    uint64_t now = get_timestamp();
    if(now - time_us > 40000)
    {
        DLog(@"Warning, got an old gyro sample - timestamp %lld, now %lld\n", time_us, now);
        return;
    }
    dispatch_async(inputQueue, ^{
        if (!isSensorFusionRunning) return;
        uint64_t time = gyroData.timestamp * 1000000;
        [self flushOperationsBeforeTime:time - 40000];
        [self enqueueOperation:[[RCSensorFusionOperation alloc] initWithBlock:^{
            if(!isSensorFusionRunning) return;
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

#pragma mark - QR Code handling for ViewAR
- (RCTransformation *) getTransformationForQRCodeObservation:(AVMetadataMachineReadableCodeObject *)observation transformOutput:(bool)transform
{
    [QRSync observeQR:observation];
    return nil;
}

@end
