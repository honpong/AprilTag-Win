//
//  TMCorVisManagerFactory.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCorvisManagerFactory.h"
#import "RCAVSessionManagerFactory.h"
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

@interface RCCorvisManagerImpl : NSObject <RCCorvisManager>
{
    struct mapbuffer *_databuffer;
    dispatch_t *_databuffer_dispatch;
    filter_setup *_cor_setup;
    bool isPluginsStarted;
    bool didReset;
}

- (void)setupPluginsWithFilter:(bool)filter withCapture:(bool)capture withReplay:(bool)replay withLocationValid:(bool)locationValid withLatitude:(double)latitude withLongitude:(double)longitude withAltitude:(double)altitude withStatusCallback:(filterStatusCallback)_statusCallback;
- (void)teardownPlugins;
- (void)startPlugins;
- (void)stopPlugins;
- (void)startMeasurement;
- (void)stopMeasurement;
- (void)receiveVideoFrame:(unsigned char*)pixel withWidth:(uint32_t)width withHeight:(uint32_t)height withTimestamp:(CMTime)timestamp;
- (void)receiveAccelerometerData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z;
- (void)receiveGyroData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z;
@end

@implementation RCCorvisManagerImpl

filterStatusCallback statusCallback;
struct corvis_device_parameters finalDeviceParameters;
bool parametersGood;

- (id)init
{
    self = [super init];
    
    if (self)
    {
        NSLog(@"CorvisManager init");
        isPluginsStarted = NO;
        didReset = false;
    }
    
    return self;
}

- (void)filterCallback
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
        if(statusCallback) //in case we get scheduled after teardownplugins
            statusCallback(measuring, x, stdx, y, stdy, z, stdz, path, stdpath, rx, stdrx, ry, stdry, rz, stdrz, orientx, orienty, failureCode, converged, steady, aligned, speedwarn, visionwarn, visionfail, speedfail, otherfail);
    });
}

void filter_callback_proxy(void *self)
{
    [(__bridge id)self filterCallback];
}

- (void)setupPluginsWithFilter:(bool)filter withCapture:(bool)capture withReplay:(bool)replay withLocationValid:(bool)locationValid withLatitude:(double)latitude withLongitude:(double)longitude withAltitude:(double)altitude withStatusCallback:(filterStatusCallback)_statusCallback
{
    NSLog(@"CorvisManager.setupPlugins");
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
    _databuffer_dispatch->max_latency = 25000;
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
        statusCallback = _statusCallback;
    } else _cor_setup = NULL;
}

- (void)teardownPlugins
{
    NSLog(@"CorvisManager.teardownPlugins");
    
    delete _databuffer_dispatch;
    delete _databuffer;
    if (_cor_setup) delete _cor_setup;
    plugins_clear();
    statusCallback = nil;
}

- (void)startPlugins
{
    NSLog(@"CorvisManager.startPlugins");
    
    if (!isPluginsStarted)
    {
        cor_time_init();
        plugins_start();
        
        isPluginsStarted = YES;
    }
}

- (void)stopPlugins
{
    NSLog(@"CorvisManager.stopPlugins");
    
    if (isPluginsStarted)
    {
        isPluginsStarted = NO;
        plugins_stop();
    }
}

- (BOOL)isPluginsStarted
{
    return isPluginsStarted;
}

- (void)sendControlPacket:(uint16_t)state
{
    if (isPluginsStarted)
    {
        uint64_t time_us = get_timestamp();
        packet_t *buf = mapbuffer_alloc(_databuffer, packet_filter_control, 0);
        buf->header.user = state;
        mapbuffer_enqueue(_databuffer, buf, time_us);
    }
}

- (void)startMeasurement
{
    [self sendControlPacket:1];
}

- (void)stopMeasurement
{
    finalDeviceParameters = _cor_setup->get_device_parameters();
    parametersGood = (_cor_setup->get_filter_converged() >= 1.) && !_cor_setup->get_failure_code();
    [self sendControlPacket:0];
}

- (void)saveDeviceParameters
{
    if(parametersGood) [RCCalibration saveCalibrationData:finalDeviceParameters];
}

- (void)sendResetPacket
{
    [self sendControlPacket:2];
}

- (void)receiveVideoFrame:(unsigned char*)pixel withWidth:(uint32_t)width withHeight:(uint32_t)height withTimestamp:(CMTime)timestamp
{
    if (isPluginsStarted)
    {
        if(![[RCAVSessionManagerFactory getAVSessionManagerInstance] isImageClean]) {
            [self sendResetPacket];
            return;
        }
        packet_t *buf = mapbuffer_alloc(_databuffer, packet_camera, width*height + 16); // 16 bytes for pgm header
    
        sprintf((char *)buf->data, "P5 %4d %3d %d\n", width, height, 255);
        unsigned char *outbase = buf->data + 16;
        memcpy(outbase, pixel, width*height);
        
        uint64_t time_us = timestamp.value / (timestamp.timescale / 1000000.);
        mapbuffer_enqueue(_databuffer, buf, time_us);
    }
}

- (void)receiveAccelerometerData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z
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

- (void)receiveGyroData:(double)timestamp withX:(double)x withY:(double)y withZ:(double)z
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

@end

@implementation RCCorvisManagerFactory

static id<RCCorvisManager> instance;

+ (id<RCCorvisManager>)getCorvisManagerInstance
{
    if (instance == nil)
    {
        instance = [[RCCorvisManagerImpl alloc] init];
    }
    
    return instance;
}

//for testing. you can set this factory to return a mock object.
+ (void)setCorvisManagerInstance:(id<RCCorvisManager>)mockObject
{
    instance = mockObject;
}

@end
