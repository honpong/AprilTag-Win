//
//  RCSensorFusion.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCSensorFusion.h"
#import "RCAVSessionManager.h"
extern "C" {
#import "cor.h"
}
#include "filter_setup.h"
#include <mach/mach_time.h>
#import "RCCalibration.h"
#import "RCHTTPClient.h"
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

@implementation RCSensorFusionError

- (id) initWithCode:(NSInteger)code withSpeed:(bool)speed withVision:(bool)vision withOther:(bool)other
{
    NSDictionary *errorDict = [NSDictionary dictionaryWithObjectsAndKeys: [NSNumber numberWithBool:vision], @"vision", [NSNumber numberWithBool:speed], @"speed", [NSNumber numberWithBool:other], @"other", nil];
    if(self = [super initWithDomain:@"com.realitycap.sensorfusion" code:code userInfo:errorDict])
    {
        _speed = speed;
        _vision = vision;
        _other = other;
    }
    return self;
}

@end

@implementation RCSensorFusion
{
    struct mapbuffer *_databuffer;
    dispatch_t *_databuffer_dispatch;
    filter_setup *_cor_setup;
    bool isSensorFusionRunning;
    bool didReset;
    CVPixelBufferRef pixelBufferCached;
    dispatch_queue_t queue, inputQueue;
    NSMutableArray *dataWaiting;
    bool use_mapbuffer;
    uint64_t lastCallbackTime;
}

#define minimumCallbackInterval 100000

- (void) validateLicense:(NSString*)apiKey withCompletionBlock:(void (^)(int, int))completionBlock withErrorBlock:(void (^)(NSError*))errorBlock
{
    if (apiKey == nil || apiKey.length == 0)
    {
        if (errorBlock) errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:1 userInfo:[NSDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. API key was nil or zero length.", NSLocalizedDescriptionKey, @"API key was nil or zero length.", NSLocalizedFailureReasonErrorKey, nil]]);
        return;
    }
    
    NSString* bundleId = [[NSBundle mainBundle] bundleIdentifier];
    if (bundleId == nil || bundleId.length == 0)
    {
        if (errorBlock) errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:2 userInfo:[NSDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. Could not get bundle ID.", NSLocalizedDescriptionKey, @"Could not get bundle ID.", NSLocalizedFailureReasonErrorKey, nil]]);
        return;
    }
    
    NSString* vendorId = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
    if (vendorId == nil || vendorId.length == 0)
    {
        if (errorBlock) errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:3 userInfo:[NSDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. Could not get ID for vendor.", NSLocalizedDescriptionKey, @"Could not get ID for vendor.", NSLocalizedFailureReasonErrorKey, nil]]);
        return;
    }

    NSDictionary *params = [NSDictionary dictionaryWithObjectsAndKeys:
                            @"3DK_TR", @"requested_resource",
                            apiKey, @"api_key",
                            bundleId, @"bundle_id",
                            vendorId, @"vendor_id",
                            nil];
    
    RCHTTPClient *client = [RCHTTPClient sharedInstance];
    
    [client
     postPath:API_LICENSING_POST
     parameters:params
     success:^(AFHTTPRequestOperation *operation, id JSON)
     {
         DLog(@"License completion %i\n%@", operation.response.statusCode, operation.responseString);
         if (JSON == nil)
         {
             if (errorBlock) errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:4 userInfo:[NSDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. Response body was empty.", NSLocalizedDescriptionKey, @"Response body was empty.", NSLocalizedFailureReasonErrorKey, nil]]);
             return;
         }
         
         NSError* serializationError;
         NSDictionary *response = [NSJSONSerialization JSONObjectWithData:JSON options:NSJSONWritingPrettyPrinted error:&serializationError];
         if (response == nil || serializationError)
         {
             if (errorBlock)
             {
                 NSMutableDictionary* userInfo = [NSMutableDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. Failed to deserialize response.", NSLocalizedDescriptionKey, @"Failed to deserialize response.", NSLocalizedFailureReasonErrorKey, nil];
                 if (serializationError) [userInfo setObject:serializationError forKey:NSUnderlyingErrorKey];
                 errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:5 userInfo:userInfo]);
             }
             return;
         }
         
         NSNumber* licenseStatus = [response objectForKey:@"license_status"];
         NSNumber* licenseType = [response objectForKey:@"license_type"];
         
         if (licenseStatus == nil || licenseType == nil)
         {
             if (errorBlock) errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:6 userInfo:[NSDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. Invalid response from server.", NSLocalizedDescriptionKey, @"Invalid response from server.", NSLocalizedFailureReasonErrorKey, nil]]);
             return;
         }
         
         if (completionBlock) completionBlock([licenseType intValue], [licenseStatus intValue]);
     }
     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         DLog(@"License failure: %i\n%@", operation.response.statusCode, operation.responseString);
         if (errorBlock)
         {
             NSMutableDictionary* userInfo = [NSMutableDictionary dictionaryWithObjectsAndKeys:@"Failed to validate license. HTTPS request failed.", NSLocalizedDescriptionKey, @"HTTPS request failed. See underlying error.", NSLocalizedFailureReasonErrorKey, nil];
             if (error) [userInfo setObject:error forKey:NSUnderlyingErrorKey];
             errorBlock([NSError errorWithDomain:ERROR_DOMAIN code:7 userInfo:userInfo]);
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
        isSensorFusionRunning = NO;
        didReset = false;
        dataWaiting = [NSMutableArray arrayWithCapacity:10];
        queue = dispatch_queue_create("com.realitycap.sensorfusion", DISPATCH_QUEUE_SERIAL);
        inputQueue = dispatch_queue_create("com.realitycap.sensorfusion.input", DISPATCH_QUEUE_SERIAL);
        use_mapbuffer = false;
        lastCallbackTime = 0;
        
        [RCHTTPClient initWithBaseUrl:API_BASE_URL withAcceptHeader:API_HEADER_ACCEPT withApiVersion:API_VERSION];
    }
    
    return self;
}

- (BOOL) isEvaluationExpired
{
    NSDateComponents *comps = [[NSDateComponents alloc] init];
    [comps setDay:29];
    [comps setMonth:9];
    [comps setYear:2013];
    NSCalendar *gregorian = [[NSCalendar alloc] initWithCalendarIdentifier:NSGregorianCalendar];
    NSDate *expires = [gregorian dateFromComponents:comps];
    NSDate* now = [NSDate date];
    
    return [now timeIntervalSinceDate:expires] > 0 ? true : false;
}

- (void) startInertialOnlyFusion
{
    LOGME
    if(isSensorFusionRunning) return;
    
    if ([self isEvaluationExpired])
    {
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"RealityCap SDK evaluation expired"
                                                        message:nil
                                                       delegate:nil
                                              cancelButtonTitle:@"OK"
                                              otherButtonTitles:nil];
        
        [alert show];
        return;
    }
    
    [self
            setupPluginsWithFilter:true
            withCapture:false
            withReplay:false
    ];

    cor_time_init();
    plugins_start();
    isSensorFusionRunning = true;
//    [MOTION_MANAGER startMotionCapture];
//    [VIDEO_MANAGER startVideoCapture];
}

- (void) setLocation:(CLLocation*)location
{
    if(location)
    {
        dispatch_async(queue, ^{ filter_compute_gravity(&_cor_setup->sfm, location.coordinate.latitude, location.altitude); } );
    }
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

- (void) startProcessingVideo
{
    dispatch_async(queue, ^{
        filter_start_processing_video(&_cor_setup->sfm);
    });
}

- (void) stopProcessingVideo
{
    [self saveCalibration];
    dispatch_async(queue, ^{
        filter_stop_processing_video(&_cor_setup->sfm);
    });
}

- (void) selectUserFeatureWithX:(float)x withY:(float)y
{
    if(!isSensorFusionRunning) return;
    dispatch_async(queue, ^{ filter_select_feature(&_cor_setup->sfm, x, y); });
}

- (void) stopSensorFusion
{
    LOGME
    if(!isSensorFusionRunning) return;
//    [VIDEO_MANAGER stopVideoCapture];
//    [MOTION_MANAGER stopMotionCapture];
    dispatch_sync(inputQueue, ^{
        isSensorFusionRunning = false;
        [self saveCalibration];
        dispatch_sync(queue, ^{});

        plugins_stop();
        [self teardownPlugins];
    });
}

- (void) resetSensorFusion
{
    LOGME;
    dispatch_sync(inputQueue, ^{
        if(!isSensorFusionRunning) return;
        dispatch_async(queue, ^{ filter_reset_full(&_cor_setup->sfm); });
    });
}

- (void) filterCallbackWithSampleBuffer:(CMSampleBufferRef)sampleBuffer
{
    //perform these operations synchronously in the calling (filter) thread
    int failureCode = _cor_setup->get_failure_code();
    struct filter *f = &(_cor_setup->sfm);
    float converged = _cor_setup->get_filter_converged();

    bool
        steady = _cor_setup->get_device_steady(),
        visionfail = _cor_setup->get_vision_failure(),
        speedfail = _cor_setup->get_speed_failure(),
        otherfail = _cor_setup->get_other_failure();
    
    RCSensorFusionStatus* status = [[RCSensorFusionStatus alloc] initWithProgress:converged withStatusCode:failureCode withIsSteady:steady];
    RCTranslation* translation = [[RCTranslation alloc] initWithVector:f->s.T.v withStandardDeviation:v4_sqrt(f->s.T.variance)];
    RCRotation* rotation = [[RCRotation alloc] initWithVector:f->s.W.v withStandardDeviation:v4_sqrt(f->s.W.variance)];
    RCTransformation* transformation = [[RCTransformation alloc] initWithTranslation:translation withRotation:rotation];

    RCTranslation* camT = [[RCTranslation alloc] initWithVector:f->s.Tc.v withStandardDeviation:v4_sqrt(f->s.Tc.variance)];
    RCRotation* camR = [[RCRotation alloc] initWithVector:f->s.Wc.v withStandardDeviation:v4_sqrt(f->s.Wc.variance)];
    RCTransformation* camTransform = [[RCTransformation alloc] initWithTranslation:camT withRotation:camR];
    
    RCScalar *totalPath = [[RCScalar alloc] initWithScalar:f->s.total_distance withStdDev:0.];
    
    RCCameraParameters *camParams = [[RCCameraParameters alloc] initWithFocalLength:f->s.focal_length.v withOpticalCenterX:f->s.center_x.v withOpticalCenterY:f->s.center_y.v withRadialSecondDegree:f->s.k1.v withRadialFourthDegree:f->s.k2.v];

    RCSensorFusionData* data = [[RCSensorFusionData alloc] initWithStatus:status withTransformation:transformation withCameraTransformation:camTransform withCameraParameters:camParams withTotalPath:totalPath withFeatures:[self getFeaturesArray] withSampleBuffer:sampleBuffer];

    //send the callback to the main/ui thread
    dispatch_async(dispatch_get_main_queue(), ^{
        if ([self.delegate respondsToSelector:@selector(sensorFusionDidUpdate:)]) [self.delegate sensorFusionDidUpdate:data];
        if(speedfail || visionfail || otherfail) {
            RCSensorFusionError *error =[ [RCSensorFusionError alloc] initWithCode:failureCode withSpeed:speedfail withVision:visionfail withOther:otherfail];
            if ([self.delegate respondsToSelector:@selector(sensorFusionError:)]) [self.delegate sensorFusionError:error];
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
            //TODO: fix this - it's the wrong standard deviation
            f_t logstd = sqrt(i->variance);
            f_t rho = exp(i->v);
            f_t drho = exp(i->v + logstd);
            f_t stdev = drho - rho;
            
            RCFeaturePoint* feature = [[RCFeaturePoint alloc] initWithId:i->id withX:i->current[0] withY:i->current[1] withDepth:[[RCScalar alloc] initWithScalar:i->depth withStdDev:stdev] withWorldPoint:[[RCPoint alloc]initWithX:i->world[0] withY:i->world[1] withZ:i->world[2]] withInitialized:(i->status == feature_normal)];
            [array addObject:feature];
        }
    }
    return [NSArray arrayWithArray:array];
}

- (void) setupPluginsWithFilter:(bool)filter
                    withCapture:(bool)capture
                     withReplay:(bool)replay
{
    LOGME
    if(capture || replay || !filter) use_mapbuffer = true;
    if(use_mapbuffer) {
        _databuffer = new mapbuffer();
        _databuffer_dispatch = new dispatch_t();
        _databuffer_dispatch->mb = _databuffer;
        if(capture  || (!capture && !replay)) {
            _databuffer_dispatch->threaded = true;
            _databuffer->block_when_full = true;
        } else {
            if(replay) _databuffer->replay = true;
            _databuffer->dispatch = _databuffer_dispatch;
        }
        _databuffer_dispatch->reorder_depth = 20;
        _databuffer_dispatch->max_latency = 40000;
        NSString *documentPath = [DOCS_DIRECTORY stringByAppendingPathComponent:@"latest"];
        const char *filename = [documentPath cStringUsingEncoding:NSUTF8StringEncoding];
        NSString *solutionPath = [DOCS_DIRECTORY stringByAppendingPathComponent:@"latest_solution"];
        const char *outname = [solutionPath cStringUsingEncoding:NSUTF8StringEncoding];
        if(replay || capture) _databuffer->filename = filename;
        else _databuffer->filename = NULL;
        _databuffer->size = 32 * 1024 * 1024;
        
        struct plugin mbp = mapbuffer_open(_databuffer);
        plugins_register(mbp);
        struct plugin disp = dispatch_init(_databuffer_dispatch);
        plugins_register(disp);
        if(filter) {
            corvis_device_parameters dc = [RCCalibration getCalibrationData];
            _cor_setup = new filter_setup(_databuffer_dispatch, outname, &dc);
        } else _cor_setup = NULL;
    } else {
        corvis_device_parameters dc = [RCCalibration getCalibrationData];
        _cor_setup = new filter_setup(&dc);
    }
}

- (void) teardownPlugins
{
    LOGME
    if(_databuffer_dispatch) delete _databuffer_dispatch;
    if(_databuffer) delete _databuffer;
    if(_cor_setup) delete _cor_setup;
    plugins_clear();
}

- (BOOL) isSensorFusionRunning
{
    return isSensorFusionRunning;
}

- (void) sendControlPacket:(uint16_t)state
{
    dispatch_sync(inputQueue, ^{
        if (isSensorFusionRunning) {
            uint64_t time_us = get_timestamp();
            packet_t *buf = mapbuffer_alloc(_databuffer, packet_filter_control, 0);
            buf->header.user = state;
            mapbuffer_enqueue(_databuffer, buf, time_us);
        }
    });
}

- (void) resetOrigin
{
    LOGME
    if(use_mapbuffer) {
        [self sendControlPacket:1];
    } else {
        dispatch_async(queue, ^{
            filter_set_reference(&_cor_setup->sfm);
        });
    }
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
        DLog( @"sample buffer is not ready. Skipping sample" );
        return;
    }
    CFRetain(sampleBuffer);
    CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);
    CVPixelBufferRetain(pixelBuffer);

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

        uint32_t width = CVPixelBufferGetWidth(pixelBuffer);
        uint32_t height = CVPixelBufferGetHeight(pixelBuffer);
        uint64_t time_us = timestamp.value / (timestamp.timescale / 1000000.);

        CVPixelBufferLockBaseAddress(pixelBuffer, 0);
        unsigned char *pixel = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer,0);

        if(use_mapbuffer) {
            packet_t *buf = mapbuffer_alloc(_databuffer, packet_camera, width*height + 16); // 16 bytes for pgm header
    
            sprintf((char *)buf->data, "P5 %4d %3d %d\n", width, height, 255);
            unsigned char *outbase = buf->data + 16;
            memcpy(outbase, pixel, width*height);
            CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
            CVPixelBufferRelease(pixelBuffer);
            CFRelease(sampleBuffer);
            mapbuffer_enqueue(_databuffer, buf, time_us);
        } else {
            uint64_t offset_time = time_us + 16667;
            [self flushOperationsBeforeTime:offset_time];
            dispatch_async(queue, ^{
                if(filter_image_measurement(&_cor_setup->sfm, pixel, width, height, offset_time)) {
                    if(pixelBufferCached) {
                        CVPixelBufferUnlockBaseAddress(pixelBufferCached, 0);
                        CVPixelBufferRelease(pixelBufferCached);
                    }
                    pixelBufferCached = pixelBuffer;
                    //sampleBuffer is released in filterCallback's block
                    [self filterCallbackWithSampleBuffer:sampleBuffer];
                } else {
                    CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
                    CVPixelBufferRelease(pixelBuffer);
                    CFRelease(sampleBuffer);
                    [self filterCallbackWithSampleBuffer:nil];
                }
            });
        }
    });
}

- (void) receiveAccelerometerData:(CMAccelerometerData *)accelerationData;
{
    if(!isSensorFusionRunning) return;
    dispatch_async(inputQueue, ^{
        if (!isSensorFusionRunning) return;
        if(use_mapbuffer) {
            packet_t *p = mapbuffer_alloc(_databuffer, packet_accelerometer, 3*4);
            ((float*)p->data)[0] = -accelerationData.acceleration.x * 9.80665;
            ((float*)p->data)[1] = -accelerationData.acceleration.y * 9.80665;
            ((float*)p->data)[2] = -accelerationData.acceleration.z * 9.80665;
            mapbuffer_enqueue(_databuffer, p, accelerationData.timestamp * 1000000);
        } else {
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
        }
    });
}

- (void) receiveGyroData:(CMGyroData *)gyroData
{
    if(!isSensorFusionRunning) return;
    dispatch_async(inputQueue, ^{
        if (!isSensorFusionRunning) return;
        uint64_t time = gyroData.timestamp * 1000000;
        if(use_mapbuffer) {
            packet_t *p = mapbuffer_alloc(_databuffer, packet_gyroscope, 3*4);
            ((float*)p->data)[0] = gyroData.rotationRate.x;
            ((float*)p->data)[1] = gyroData.rotationRate.y;
            ((float*)p->data)[2] = gyroData.rotationRate.z;
            mapbuffer_enqueue(_databuffer, p, time);
        } else {
            [self flushOperationsBeforeTime:time - 40000];
            [self enqueueOperation:[[RCSensorFusionOperation alloc] initWithBlock:^{
                float data[3];
                data[0] = gyroData.rotationRate.x;
                data[1] = gyroData.rotationRate.y;
                data[2] = gyroData.rotationRate.z;
                filter_gyroscope_measurement(&_cor_setup->sfm, data, time);
            } withTime:time]];
        }
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
        if(use_mapbuffer) {
            packet_t *p = mapbuffer_alloc(_databuffer, packet_core_motion, 6*4);
            ((float*)p->data)[0] = motionData.rotationRate.x;
            ((float*)p->data)[1] = motionData.rotationRate.y;
            ((float*)p->data)[2] = motionData.rotationRate.z;
            ((float*)p->data)[3] = -motionData.gravity.x * 9.80665;
            ((float*)p->data)[4] = -motionData.gravity.y * 9.80665;
            ((float*)p->data)[5] = -motionData.gravity.z * 9.80665;
            mapbuffer_enqueue(_databuffer, p, time);
        } else {
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
        }
    });
}
*/
@end
