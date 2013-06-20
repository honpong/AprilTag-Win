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

@implementation RCSensorFusion
{
    struct mapbuffer *_databuffer;
    dispatch_t *_databuffer_dispatch;
    filter_setup *_cor_setup;
    bool isPluginsStarted;
    bool didReset;
    struct corvis_device_parameters finalDeviceParameters;
    bool parametersGood;
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
        isPluginsStarted = NO;
        didReset = false;
    }
    
    return self;
}

- (void) filterCallback
{
    //perform these operations synchronously in the calling (filter) thread
    int failureCode = _cor_setup->get_failure_code();
    struct filter *f = &(_cor_setup->sfm);
    float
        x = f->s.T.v[0],
        stdx = sqrt(f->s.T.variance[0]),
        y = f->s.T.v[1],
        stdy = sqrt(f->s.T.variance[1]),
        z = f->s.T.v[2],
        stdz = sqrt(f->s.T.variance[2]),
        path = f->s.total_distance,
        stdpath = 0.,
        rx = f->s.W.v[0],
        stdrx = sqrt(f->s.W.variance[0]),
        ry = f->s.W.v[1],
        stdry = sqrt(f->s.W.variance[1]),
        rz = f->s.W.v[2],
        stdrz = sqrt(f->s.W.variance[2]),
        orientx = _cor_setup->sfm.s.projected_orientation_marker.x,
        orienty = _cor_setup->sfm.s.projected_orientation_marker.y,
        converged = _cor_setup->get_filter_converged();

    bool
        measuring = f->measurement_running,
        steady = _cor_setup->get_device_steady(),
        aligned = _cor_setup->get_device_aligned(),
        speedwarn = _cor_setup->get_speed_warning(),
        visionwarn = _cor_setup->get_vision_warning(),
        visionfail = _cor_setup->get_vision_failure(),
        speedfail = _cor_setup->get_speed_failure(),
        otherfail = _cor_setup->get_other_failure();

    //if the filter has failed, reset it
    if(failureCode) filter_reset_full(f);
    
    //send the callback to the main/ui thread
    dispatch_async(dispatch_get_main_queue(), ^{
        [self.delegate didUpdateMeasurementStatus:measuring code:failureCode converged:converged steady:steady aligned:aligned speed_warning:speedwarn vision_warning:visionwarn vision_failure:visionfail speed_failure:speedfail other_failure:otherfail];
        [self.delegate didUpdatePose:orientx withY:orienty];
        if (measuring) [self.delegate didUpdateMeasurementData:x stdx:stdx y:y stdy:stdy z:z stdz:stdz path:path stdpath:stdpath rx:rx stdrx:stdrx ry:ry stdry:stdry rz:rz stdrz:stdrz];
    });
}

void filter_callback_proxy(void *self)
{
    [(__bridge id)self filterCallback];
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
    NSArray  *documentDirList = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentDir  = [documentDirList objectAtIndex:0];
    NSString *documentPath = [documentDir stringByAppendingPathComponent:@"latest"];
    const char *filename = [documentPath cStringUsingEncoding:NSUTF8StringEncoding];
    NSString *solutionPath = [documentDir stringByAppendingPathComponent:@"latest_solution"];
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
        _cor_setup->sfm.measurement_callback = filter_callback_proxy;
        _cor_setup->sfm.measurement_callback_object = (__bridge void *)self;
    } else _cor_setup = NULL;
}

- (void) teardownPlugins
{
    LOGME
    delete _databuffer_dispatch;
    delete _databuffer;
    if (_cor_setup) delete _cor_setup;
    plugins_clear();
    self.delegate = nil;
}

- (void) startPlugins
{
    LOGME;
    if (!isPluginsStarted)
    {
        cor_time_init();
        plugins_start();
        
        isPluginsStarted = YES;
    }
}

- (void) stopPlugins
{
    LOGME
    if (isPluginsStarted)
    {
        isPluginsStarted = NO;
        plugins_stop();
    }
}

- (BOOL) isPluginsStarted
{
    return isPluginsStarted;
}

- (void) sendControlPacket:(uint16_t)state
{
    if (isPluginsStarted)
    {
        uint64_t time_us = get_timestamp();
        packet_t *buf = mapbuffer_alloc(_databuffer, packet_filter_control, 0);
        buf->header.user = state;
        mapbuffer_enqueue(_databuffer, buf, time_us);
    }
}

- (void) startMeasurement
{
    [self sendControlPacket:1];
    [self.delegate didUpdateMeasurementData:0 stdx:0 y:0 stdy:0 z:0 stdz:0 path:0 stdpath:0 rx:0 stdrx:0 ry:0 stdry:0 rz:0 stdrz:0];
}

- (void) stopMeasurement
{
    finalDeviceParameters = _cor_setup->get_device_parameters();
    parametersGood = (_cor_setup->get_filter_converged() >= 1.) && !_cor_setup->get_failure_code();
    [self sendControlPacket:0];
}

- (void) saveDeviceParameters
{
    if(parametersGood) [RCCalibration saveCalibrationData:finalDeviceParameters];
}

- (void) sendResetPacket
{
    [self sendControlPacket:2];
}

- (void) receiveVideoFrame:(CMSampleBufferRef)sampleBuffer
{
    if (isPluginsStarted)
    {
        /*if(![[RCAVSessionManager sharedInstance] isImageClean]) {
            [self sendResetPacket];
            return;
        }*/

        if(!CMSampleBufferDataIsReady(sampleBuffer) )
        {
            NSLog( @"sample buffer is not ready. Skipping sample" );
            return;
        }

        CMTime timestamp = (CMTime)CMSampleBufferGetPresentationTimeStamp(sampleBuffer);

        //capture image meta data
        //        CFDictionaryRef metadataDict = CMGetAttachment(sampleBuffer, kCGImagePropertyExifDictionary , NULL);
        //        NSLog(@"metadata: %@", metadataDict);

        CVPixelBufferRef pixelBuffer = (CVPixelBufferRef)CMSampleBufferGetImageBuffer(sampleBuffer);

        uint32_t width = CVPixelBufferGetWidth(pixelBuffer);
        uint32_t height = CVPixelBufferGetHeight(pixelBuffer);

        CVPixelBufferLockBaseAddress(pixelBuffer, 0);
        unsigned char *pixel = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer,0);
        CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);

        packet_t *buf = mapbuffer_alloc(_databuffer, packet_camera, width*height + 16); // 16 bytes for pgm header
    
        sprintf((char *)buf->data, "P5 %4d %3d %d\n", width, height, 255);
        unsigned char *outbase = buf->data + 16;
        memcpy(outbase, pixel, width*height);
        
        uint64_t time_us = timestamp.value / (timestamp.timescale / 1000000.);
        mapbuffer_enqueue(_databuffer, buf, time_us);
    }
}

- (void) receiveAccelerometerData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z
{
    if (isPluginsStarted)
    {
        packet_t *p = mapbuffer_alloc(_databuffer, packet_accelerometer, 3*4);
        //ios gives acceleration in g-units, so multiply by standard gravity in m/s^2
        //it appears that accelerometer axes are flipped
        ((float*)p->data)[0] = -x * 9.80665;
        ((float*)p->data)[1] = -y * 9.80665;
        ((float*)p->data)[2] = -z * 9.80665;
        mapbuffer_enqueue(_databuffer, p, timestamp * 1000000);
    }
}

- (void) receiveGyroData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z
{
    if (isPluginsStarted)
    {
        packet_t *p = mapbuffer_alloc(_databuffer, packet_gyroscope, 3*4);
        ((float*)p->data)[0] = x;
        ((float*)p->data)[1] = y;
        ((float*)p->data)[2] = z;
        mapbuffer_enqueue(_databuffer, p, timestamp * 1000000);
    }
}

- (int) getCurrentFeatures:(struct corvis_feature_info *)features withMax:(int)max
{
    if(_cor_setup)
        return filter_get_features(&_cor_setup->sfm, features, max);
    else
        return 0;
}

- (void) getCurrentCameraMatrix:(float [16])matrix withFocalCenterRadial:(float [5])focalCenterRadial withVirtualTapeStart:(float *)start
{
    if(_cor_setup) {
        filter_get_camera_parameters(&_cor_setup->sfm, matrix, focalCenterRadial);
        for(int i = 0; i < 3; i++) start[i] = _cor_setup->sfm.s.virtual_tape_start[i];
    }
}

@end
