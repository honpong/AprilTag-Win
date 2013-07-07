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

- (id) initWithBlock:(void (^)())block withTime:(uint64_t)time
{
    if(self = [super init]) {
        self.block = block;
        self.time = time;
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
    uint64_t lastVideoTime;
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
    }
    
    return self;
}

- (void) startSensorFusion:(CLLocation*)location
{
    LOGME

    [self
            setupPluginsWithFilter:true
            withCapture:false
            withReplay:false
            withLocationValid:location ? true : false
            withLatitude:location ? location.coordinate.latitude : 0
            withLongitude:location ? location.coordinate.longitude : 0
            withAltitude:location ? location.altitude : 0
    ];

    cor_time_init();
    plugins_start();
    isSensorFusionRunning = true;
//    [MOTION_MANAGER startMotionCapture];
//    [VIDEO_MANAGER startVideoCapture];
}

- (void) stopSensorFusion
{
    LOGME

//    [VIDEO_MANAGER stopVideoCapture];
//    [MOTION_MANAGER stopMotionCapture];
    dispatch_sync(inputQueue, ^{
        isSensorFusionRunning = false;
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
            NSDictionary *errorDict = [NSDictionary dictionaryWithObjectsAndKeys: [NSNumber numberWithBool:visionfail], @"vision", [NSNumber numberWithBool:speedfail], @"speed", [NSNumber numberWithBool:otherfail], @"other", nil];
            NSError *error = [NSError errorWithDomain:@"com.realitycap.sensorfusion" code:failureCode userInfo:errorDict];
            if ([self.delegate respondsToSelector:@selector(sensorFusionError:)]) [self.delegate sensorFusionError:error];
        }
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
              withLocationValid:(bool)locationValid
                   withLatitude:(double)latitude
                  withLongitude:(double)longitude
                   withAltitude:(double)altitude
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
            if(locationValid) {
                _cor_setup->sfm.location_valid = true;
                _cor_setup->sfm.latitude = latitude;
                _cor_setup->sfm.longitude = longitude;
                _cor_setup->sfm.altitude = altitude;
            }
        } else _cor_setup = NULL;
    } else {
        corvis_device_parameters dc = [RCCalibration getCalibrationData];
        _cor_setup = new filter_setup(&dc);
        if(locationValid) {
            _cor_setup->sfm.location_valid = true;
            _cor_setup->sfm.latitude = latitude;
            _cor_setup->sfm.longitude = longitude;
            _cor_setup->sfm.altitude = altitude;
        }
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

- (void) markStart
{
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
    __block struct corvis_device_parameters finalDeviceParameters;
    __block bool parametersGood;
    dispatch_sync(queue, ^{
        finalDeviceParameters = _cor_setup->get_device_parameters();
        parametersGood = (_cor_setup->get_filter_converged() >= 1.) && !_cor_setup->get_failure_code();
    });
    if(parametersGood) [RCCalibration saveCalibrationData:finalDeviceParameters];
    return parametersGood;
}

- (void) receiveVideoFrame:(CMSampleBufferRef)sampleBuffer
{
    if(!CMSampleBufferDataIsReady(sampleBuffer) )
    {
        NSLog( @"sample buffer is not ready. Skipping sample" );
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
        //        NSLog(@"metadata: %@", metadataDict);

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
                filter_image_measurement(&_cor_setup->sfm, pixel, width, height, offset_time);
                if(pixelBufferCached) {
                    CVPixelBufferUnlockBaseAddress(pixelBufferCached, 0);
                    CVPixelBufferRelease(pixelBufferCached);
                }
                pixelBufferCached = pixelBuffer;
                //sampleBuffer is released in filterCallback's block
                [self filterCallbackWithSampleBuffer:sampleBuffer];
            });
            lastVideoTime = offset_time;
        }
    });
}

- (void) receiveAccelerometerData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z
{
    dispatch_async(inputQueue, ^{
        if (!isSensorFusionRunning) return;
        if(use_mapbuffer) {
            packet_t *p = mapbuffer_alloc(_databuffer, packet_accelerometer, 3*4);
            ((float*)p->data)[0] = -x * 9.80665;
            ((float*)p->data)[1] = -y * 9.80665;
            ((float*)p->data)[2] = -z * 9.80665;
            mapbuffer_enqueue(_databuffer, p, timestamp * 1000000);
        } else {
            uint64_t time = timestamp * 1000000;
            [self enqueueOperation:[[RCSensorFusionOperation alloc] initWithBlock:^{
                float data[3];
                //ios gives acceleration in g-units, so multiply by standard gravity in m/s^2
                //it appears that accelerometer axes are flipped
                data[0] = -x * 9.80665;
                data[1] = -y * 9.80665;
                data[2] = -z * 9.80665;
                filter_accelerometer_measurement(&_cor_setup->sfm, data, time);
            } withTime:time]];
        }
    });
}

- (void) receiveGyroData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z
{
    dispatch_async(inputQueue, ^{
        if (!isSensorFusionRunning) return;
        if(use_mapbuffer) {
            packet_t *p = mapbuffer_alloc(_databuffer, packet_gyroscope, 3*4);
            ((float*)p->data)[0] = x;
            ((float*)p->data)[1] = y;
            ((float*)p->data)[2] = z;
            mapbuffer_enqueue(_databuffer, p, timestamp * 1000000);
        } else {
            uint64_t time = timestamp * 1000000;
            [self enqueueOperation:[[RCSensorFusionOperation alloc] initWithBlock:^{
                float data[3];
                data[0] = x;
                data[1] = y;
                data[2] = z;
                filter_gyroscope_measurement(&_cor_setup->sfm, data, time);
            } withTime:time]];
        }
    });
}

@end
