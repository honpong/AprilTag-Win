//
//  RCSensorFusion.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCSensorFusion.h"
#import "RCSensorData.h"
#include "sensor_fusion_queue.h"
#include "filter_setup.h"
#include <mach/mach_time.h>
#import "RCCalibration.h"
#import "RCPrivateHTTPClient.h"
#import "NSString+RCString.h"
#include <functional>
#include <memory>
#import "RCLicenseValidator.h"
#import <ImageIO/ImageIO.h>

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

#pragma mark -

@interface RCSensorFusion ()

@property (nonatomic) RCLicenseType licenseType;

@end

@implementation RCSensorFusion
{
    filter_setup *_cor_setup;
    bool isSensorFusionRunning;
    bool isProcessingVideo;
    bool processingVideoRequested;
    std::unique_ptr<fusion_queue> queue;
    BOOL isLicenseValid;
    bool isStableStart;
    RCSensorFusionRunState lastRunState;
    RCSensorFusionErrorCode lastErrorCode;
    RCSensorFusionConfidence lastConfidence;
    float lastProgress;
    NSString* licenseKey;
}

- (void) setLicenseKey:(NSString*)licenseKey_
{
    licenseKey = licenseKey_;
}

// this is still exposed to library clients for backward compatibility. calling it validates the license, but does NOT prevent the library from doing it's own check when starting up.
- (void) validateLicense:(NSString*)apiKey withCompletionBlock:(void (^)(int licenseType, int licenseStatus))completionBlock withErrorBlock:(void (^)(NSError*))errorBlock
{
#ifdef OFFLINE // don't even construct a RCLicenseValidator, because it uses RCPrivateHTTPClient, which we can't use because OFFLINE also means no encryption
    if (completionBlock) completionBlock(RCLicenseTypeFull, RCLicenseStatusOK);
    return;
#else
    RCLicenseValidator* validator = [RCLicenseValidator initWithBundleId:[[NSBundle mainBundle] bundleIdentifier] withVendorId:[[[UIDevice currentDevice] identifierForVendor] UUIDString] withHTTPClient:[RCPrivateHTTPClient sharedInstance] withUserDefaults:NSUserDefaults.standardUserDefaults];
    
#ifdef LAX_LICENSE_VALIDATION
    validator.licenseRule = RCLicenseRuleLax;
#endif
    
#ifdef VIEWAR_ONLY
    validator.licenseRule = RCLicenseRuleBundleID;
    validator.allowBundleID = @"com.viewar.kareshopguid";
#endif

    [validator validateLicense:apiKey withCompletionBlock:completionBlock withErrorBlock:errorBlock];
#endif
}

- (void) validateLicenseInternal
{
    [self validateLicense:licenseKey withCompletionBlock:^(int licenseType, int licenseStatus) {
        isLicenseValid = YES;
    } withErrorBlock:^(NSError* error) {
        isLicenseValid = NO;
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
            RCSensorFusionStatus* status = [[RCSensorFusionStatus alloc] initWithRunState:RCSensorFusionRunStateInactive withProgress:0 withConfidence:RCSensorFusionConfidenceNone withError:error];
            [self.delegate sensorFusionDidChangeStatus:status];
        }
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
        
        auto cam_fn = [self](camera_data &&data)
        {
            bool docallback = true;
            CMSampleBufferRef sampleBuffer = (CMSampleBufferRef)data.image_handle.get();
            if(!isSensorFusionRunning)
            {
            } else if(isProcessingVideo) {
                docallback = filter_image_measurement(&_cor_setup->sfm, data);
                [self sendStatus];
                if(docallback) [self sendDataWithSampleBuffer:sampleBuffer];
            } else {
                //We're not yet processing video, but we do want to send updates for the video preview. Make sure that rotation is initialized.
                docallback =  _cor_setup->sfm.gravity_init;
                [self sendStatus];
                if(docallback) [self sendDataWithSampleBuffer:sampleBuffer];
            }
        };
        
        auto acc_fn = [self](accelerometer_data &&data)
        {
            if(!isSensorFusionRunning) return;
            filter_accelerometer_measurement(&_cor_setup->sfm, data.accel_m__s2, data.timestamp);
            [self sendStatus];
        };

        auto gyr_fn = [self](gyro_data &&data)
        {
            if(!isSensorFusionRunning) return;
            filter_gyroscope_measurement(&_cor_setup->sfm, data.angvel_rad__s, data.timestamp);
        };

        queue = std::make_unique<fusion_queue>(cam_fn, acc_fn, gyr_fn, fusion_queue::latency_strategy::MINIMIZE_DROPS, std::chrono::microseconds(10000)); //Have to make jitter high - ipad air 2 accelerometer has high latency, we lose about 10% of samples with jitter at 8000
        lastRunState = RCSensorFusionRunStateInactive;
        lastErrorCode = RCSensorFusionErrorCodeNone;
        lastConfidence = RCSensorFusionConfidenceNone;
        lastProgress = 0.;
        licenseKey = nil;
        
#if !SKIP_LICENSE_CHECK
        [RCPrivateHTTPClient initWithBaseUrl:API_BASE_URL withAcceptHeader:API_HEADER_ACCEPT withApiVersion:API_VERSION];
#endif
        
        device_parameters dc = [RCCalibration getCalibrationData];
        _cor_setup = new filter_setup(&dc);
    }
    
    return self;
}

- (void) dealloc
{
    if(_cor_setup) delete _cor_setup;
}

- (void) startInertialOnlyFusion __attribute((deprecated("No longer needed; does nothing.")))
{
}

- (void) setLocation:(CLLocation*)location
{
    _location = location;
    
    if(location)
    {
        queue->dispatch_async([self, location]{
            filter_compute_gravity(&_cor_setup->sfm, location.coordinate.latitude, location.altitude);
        });
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
    queue->start_async(false);
    queue->dispatch_async([self]{
        [RCCalibration clearCalibrationData];
        _cor_setup->device = [RCCalibration getCalibrationData];
        filter_initialize(&_cor_setup->sfm, &_cor_setup->device);
        filter_start_static_calibration(&_cor_setup->sfm);
    });
    isSensorFusionRunning = true;
    
    [self validateLicenseInternal];
}

- (void) stopStaticCalibration __attribute((deprecated("No longer needed; does nothing.")))
{
}

- (void) focusOperationFinishedAtTime:(uint64_t)timestamp withLensPosition:(float)lens_position
{
    NSLog(@"Focus operation finished at %llu with %f, starting processing", timestamp, lens_position);
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

- (void) configureCameraWithDevice:(AVCaptureDevice *)device
{
    CMVideoDimensions sz = CMVideoFormatDescriptionGetDimensions(device.activeFormat.formatDescription);
    double frame_duration = (CMTimeGetSeconds(device.activeVideoMinFrameDuration) + CMTimeGetSeconds(device.activeVideoMaxFrameDuration))/2;
    DLog(@"Starting with %d width x %d height", sz.width, sz.height);
    device_set_resolution(&_cor_setup->device, sz.width, sz.height);
    filter_initialize(&_cor_setup->sfm, &_cor_setup->device);
}

- (void) startSensorFusionWithDevice:(AVCaptureDevice *)device
{
    if(isProcessingVideo || processingVideoRequested || isSensorFusionRunning) return;
    
    isStableStart = true;

    queue->start_async(true);

    queue->dispatch_async([self, device]{
        [self configureCameraWithDevice:device];
        filter_start_hold_steady(&_cor_setup->sfm);
    });
    
    isProcessingVideo = false;
    processingVideoRequested = true;

    _cor_setup->sfm.camera_control.init((__bridge void *)device);
    __weak typeof(self) weakself = self;
    std::function<void (uint64_t, float)> callback = [weakself](uint64_t timestamp, float position)
    {
        [weakself focusOperationFinishedAtTime:timestamp withLensPosition:position];
    };
    _cor_setup->sfm.camera_control.focus_lock_at_current_position(callback);

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

        queue->start_async(true);

        queue->dispatch_async([self, device]{
            if(device) { // replay starts with device = nil
                [self configureCameraWithDevice:device];
            }
            filter_start_dynamic(&_cor_setup->sfm);
        });
        
        isProcessingVideo = false;
        processingVideoRequested = true;
        isSensorFusionRunning = true;
        
        _cor_setup->sfm.camera_control.init((__bridge void *)device);
        __weak typeof(self) weakself = self;
        std::function<void (uint64_t, float)> callback = [weakself](uint64_t timestamp, float position)
        {
            [weakself focusOperationFinishedAtTime:timestamp withLensPosition:position];
        };
        _cor_setup->sfm.camera_control.focus_lock_at_current_position(callback);
    }
    else if ([self.delegate respondsToSelector:@selector(sensorFusionDidChangeStatus:)])
    {
        RCLicenseError* error = [RCLicenseError errorWithDomain:ERROR_DOMAIN code:RCSensorFusionErrorCodeLicense userInfo:nil]; // TODO: use real error code
        RCSensorFusionStatus *status = [[RCSensorFusionStatus alloc] initWithRunState:RCSensorFusionRunStateInactive withProgress:0. withConfidence:RCSensorFusionConfidenceNone withError:error];
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
/*- (void) selectUserFeatureWithX:(float)x withY:(float)y
{
    if(!isProcessingVideo) return;
    queue->dispatch_async([=]{
        filter_select_feature(&_cor_setup->sfm, x, y);
    });
}*/

- (void) flushAndReset
{
    isSensorFusionRunning = false;
    queue->stop_sync();
    queue->dispatch_sync([self]{
        filter_initialize(&_cor_setup->sfm, &_cor_setup->device);
    });

    _cor_setup->sfm.camera_control.focus_unlock();
    _cor_setup->sfm.camera_control.release_platform_specific_object();

    isProcessingVideo = false;
    processingVideoRequested = false;
}

- (void) stopSensorFusion
{
    LOGME
    if(!isSensorFusionRunning) return;
    if ([self saveCalibration])
    {
#ifndef OFFLINE
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
            [RCCalibration postDeviceCalibration:nil onFailure:nil];
        });
#endif
    }
    else
    {
        DLog(@">>>>>> Failed to save calibration <<<<<<");
    }
    
    [self flushAndReset];
}

- (void) pauseSensorFusionAndResetPosition
{
    queue->dispatch_async([self]{
        filter_start_inertial_only(&_cor_setup->sfm);
    });
}

- (void) unpauseSensorFusion
{
    queue->dispatch_async([self]{
        filter_start_dynamic(&_cor_setup->sfm);
    });
}

#pragma mark - RCSensorFusionDelegate handling

- (void) sendStatus
{
    //perform these operations synchronously in the calling (filter) thread
    struct filter *f = &(_cor_setup->sfm);
    
    //handle errors first, so that we don't provide invalid data. get error codes before handle_errors so that we don't reset them without reading first
    RCSensorFusionErrorCode errorCode = _cor_setup->get_error();
    float converged = _cor_setup->get_filter_converged();
    
    RCSensorFusionConfidence confidence = RCSensorFusionConfidenceNone;
    if(f->run_state == RCSensorFusionRunStateRunning)
    {
        if(errorCode == RCSensorFusionErrorCodeVision)
        {
            confidence = RCSensorFusionConfidenceLow;
        }
        else if(f->has_converged)
        {
            confidence = RCSensorFusionConfidenceHigh;
        }
        else
        {
            confidence = RCSensorFusionConfidenceMedium;
        }
    }
    
    if((converged == lastProgress) && (errorCode == lastErrorCode) && (f->run_state == lastRunState) && (confidence == lastConfidence)) return;

    // queue actions related to failures before queuing callbacks to the sdk client.
    if(errorCode == RCSensorFusionErrorCodeTooFast || errorCode == RCSensorFusionErrorCodeOther)
    {
        //Sensor fusion has already been reset by get_error, but it could have gotten random data inbetween, so do full reset
        dispatch_async(dispatch_get_main_queue(), ^{
            [self flushAndReset];
        });
    }
    else if(lastRunState == RCSensorFusionRunStateStaticCalibration && f->run_state == RCSensorFusionRunStateInactive && errorCode == RCSensorFusionErrorCodeNone)
    {
        isSensorFusionRunning = false;
        dispatch_async(dispatch_get_main_queue(), ^{
            [self stopSensorFusion];
        });
    }
    
    if((errorCode == RCSensorFusionErrorCodeVision && f->run_state != RCSensorFusionRunStateRunning)) {
        //refocus if either we tried to detect and failed, or if we've recently moved during initialization
        isProcessingVideo = false;
        processingVideoRequested = true;
        
        // Request a refocus
        __weak typeof(self) weakself = self;
        std::function<void (uint64_t, float)> callback = [weakself](uint64_t timestamp, float position)
        {
            [weakself focusOperationFinishedAtTime:timestamp withLensPosition:position];
        };
        _cor_setup->sfm.camera_control.focus_once_and_lock(callback);        
    }
    
    if((converged != lastProgress) || (errorCode != lastErrorCode) || (f->run_state != lastRunState) || (confidence != lastConfidence))
    {
        RCSensorFusionError* error = nil;
        if (errorCode != RCSensorFusionErrorCodeNone) error = [RCSensorFusionError errorWithDomain:ERROR_DOMAIN code:errorCode userInfo:nil];
        
        RCSensorFusionStatus* status = [[RCSensorFusionStatus alloc] initWithRunState:f->run_state withProgress:converged withConfidence:confidence withError:error];
        dispatch_async(dispatch_get_main_queue(), ^{
            if ([self.delegate respondsToSelector:@selector(sensorFusionDidChangeStatus:)]) [self.delegate sensorFusionDidChangeStatus:status];
        });
    }
    lastErrorCode = errorCode;
    lastRunState = f->run_state;
    lastConfidence = confidence;
    lastProgress = converged;
}

- (void) sendDataWithSampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    //perform these operations synchronously in the calling (filter) thread
    struct filter *f = &(_cor_setup->sfm);
    
    if (sampleBuffer) sampleBuffer = (CMSampleBufferRef)CFRetain(sampleBuffer);
    RCTranslation* translation = [[RCTranslation alloc] initWithVector:vFloat_from_v4(f->s.T.v) withStandardDeviation:vFloat_from_v4(v4_sqrt(f->s.T.variance()))];
    RCRotation* rotation = [[RCRotation alloc] initWithQuaternionW:(float)f->s.Q.v.w() withX:(float)f->s.Q.v.x() withY:(float)f->s.Q.v.y() withZ:(float)f->s.Q.v.z()];
    RCTransformation* transformation = [[RCTransformation alloc] initWithTranslation:translation withRotation:rotation];
    
    RCScalar *totalPath = [[RCScalar alloc] initWithScalar:f->s.total_distance withStdDev:0.];
    
    RCCameraParameters *camParams = [[RCCameraParameters alloc] initWithFocalLength:(float)f->s.focal_length.v * f->s.image_height withOpticalCenterX:(float)f->s.center_x.v * f->s.image_height + f->s.image_width / 2. - .5 withOpticalCenterY:(float)f->s.center_y.v * f->s.image_height + f->s.image_height / 2. - .5 withRadialSecondDegree:(float)f->s.k1.v withRadialFourthDegree:(float)f->s.k2.v];

    RCRotation* originRotation = [[RCRotation alloc] initWithQuaternionW:(float)f->origin.Q.w() withX:(float)f->origin.Q.x() withY:(float)f->origin.Q.y() withZ:(float)f->origin.Q.z()];
    RCTranslation* originTranslation = [[RCTranslation alloc] initWithX:(float)f->origin.T[0] withY:(float)f->origin.T[1] withZ:(float)f->origin.T[2]];
    RCTransformation * originTransform = [[RCTransformation alloc] initWithTranslation:originTranslation withRotation:originRotation];
    transformation = [originTransform composeWithTransformation:transformation];

    NSString * qrDetected = nil;
    if(f->qr.valid)
    {
        qrDetected = [NSString stringWithCString:f->qr.data.c_str() encoding:NSUTF8StringEncoding];
    }

    RCTransformation *cameraTransformation = transformation;
    RCSensorFusionData* data = [[RCSensorFusionData alloc] initWithTransformation:transformation withCameraTransformation:cameraTransformation withCameraParameters:camParams withTotalPath:totalPath withFeatures:[self getFeaturesArray] withSampleBuffer:sampleBuffer withTimestamp:sensor_clock::tp_to_micros(f->last_time) withOriginQRCode:qrDetected];

    //send the callback to the main/ui thread
    dispatch_async(dispatch_get_main_queue(), ^{
        if ([self.delegate respondsToSelector:@selector(sensorFusionDidUpdateData:)]) [self.delegate sensorFusionDidUpdateData:data];
        if(sampleBuffer) CFRelease(sampleBuffer);
    });
}

#pragma mark -

//needs to be called from the filter thread
- (NSArray*) getFeaturesArray
{
    struct filter *f = &(_cor_setup->sfm);
    NSMutableArray* array = [[NSMutableArray alloc] initWithCapacity:f->s.features.size()];
    for(list<state_vision_feature *>::iterator fiter = f->s.features.begin(); fiter != f->s.features.end(); ++fiter) {
        state_vision_feature *i = *fiter;
        if(i->is_valid()) {
            f_t stdev = i->v.stdev_meters(sqrt(i->variance()));
            
            RCFeaturePoint* feature = [[RCFeaturePoint alloc] initWithId:i->id withX:(float)i->current[0] withY:(float)i->current[1] withOriginalDepth:[[RCScalar alloc] initWithScalar:(float)i->v.depth() withStdDev:(float)stdev] withWorldPoint:[[RCPoint alloc]initWithX:(float)i->world[0] withY:(float)i->world[1] withZ:(float)i->world[2]] withInitialized:i->is_initialized()];
            [array addObject:feature];
        }
    }
    return array;
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
    device_parameters finalDeviceParameters;
    bool parametersGood;
    queue->dispatch_sync([&]{
        finalDeviceParameters = _cor_setup->get_device_parameters();
        parametersGood = !_cor_setup->get_failure_code() && !_cor_setup->sfm.calibration_bad;
    });
    if(!parametersGood) return false;
    
    return [RCCalibration saveCalibrationData:finalDeviceParameters];
}

- (void) receiveVideoFrame:(CMSampleBufferRef)sampleBuffer
{
    if(!isSensorFusionRunning) return;
    if (!sampleBuffer) return;
    if(!CMSampleBufferDataIsReady(sampleBuffer) )
    {
        DLog( @"Sample buffer is not ready. Skipping sample." );
        return;
    }
    try {
        camera_data c(camera_data_from_CMSampleBufferRef(sampleBuffer));
        queue->receive_camera(std::move(c));
    } catch (std::runtime_error) {
        //do nothing - indicates the sample / image buffer was not valid.
    }
}

- (void) receiveAccelerometerData:(CMAccelerometerData *)accelerationData;
{
    if(!isSensorFusionRunning) return;
    queue->receive_accelerometer(accelerometer_data_from_CMAccelerometerData(accelerationData));
}

- (void) receiveGyroData:(CMGyroData *)gyroData
{
    if(!isSensorFusionRunning) return;
    queue->receive_gyro(gyro_data_from_CMGyroData(gyroData));
}

#pragma mark - QR Code handling

- (void) startQRDetectionWithData:(NSString *)data withDimension:(float)dimension withAlignGravity:(bool)alignGravity
{
    std::string code;
    if(data) code = data.UTF8String;
    queue->dispatch_sync([=]() {
        filter_start_qr_detection(&_cor_setup->sfm, code, dimension, alignGravity);
    });
}

- (void) stopQRDetection
{
    queue->dispatch_sync([self]() {
        filter_stop_qr_detection(&_cor_setup->sfm);
    });
}

@end
