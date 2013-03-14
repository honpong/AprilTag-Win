//
//  TMCorVisManagerFactory.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCorvisManagerFactory.h"
extern "C" {
#import "cor.h"
}
#include "filter_setup.h"

@interface RCCorvisManagerImpl : NSObject <RCCorvisManager>
{
    struct mapbuffer *_databuffer;
    dispatch_t *_databuffer_dispatch;
    filter_setup *_cor_setup;
    bool isPluginsStarted;
}

- (void)setupPluginsWithFilter:(bool)filter withCapture:(bool)capture withReplay:(bool)replay withUpdateProgress:(void(*)(void *, float))updateProgress withUpdateMeasurement:(void(*)(void *, float, float, float, float, float, float, float, float))updateMeasurement withCallbackObject:(void *)callbackObject;
- (void)teardownPlugins;
- (void)startPlugins;
- (void)stopPlugins;
- (void)receiveVideoFrame:(unsigned char*)pixel withWidth:(uint32_t)width withHeight:(uint32_t)height withTimestamp:(CMTime)timestamp;
- (void)receiveAccelerometerData:(uint64_t)timestamp withX:(long)x withY:(long)y withZ:(long)z;
- (void)receiveGyroData:(uint64_t)timestamp withX:(long)x withY:(long)y withZ:(long)z;
@end

@implementation RCCorvisManagerImpl

- (id)init
{
    self = [super init];
    
    if (self)
    {
        NSLog(@"CorvisManager init");
        isPluginsStarted = NO;
    }
    
    return self;
}

- (void)setupPluginsWithFilter:(bool)filter withCapture:(bool)capture withReplay:(bool)replay withUpdateProgress:(void(*)(void *, float))updateProgress withUpdateMeasurement:(void(*)(void *, float, float, float, float, float, float, float, float))updateMeasurement withCallbackObject:(void *)callbackObject
{
    _databuffer = new mapbuffer();
    _databuffer_dispatch = new dispatch_t();
    if(capture) {
        _databuffer_dispatch->threaded = true;
        _databuffer_dispatch->mb = _databuffer;
    } else {
        if(replay) _databuffer->replay = true;
        _databuffer->dispatch = _databuffer_dispatch;
    }
    _databuffer_dispatch->reorder_depth = 100;
    NSArray  *documentDirList = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentDir  = [documentDirList objectAtIndex:0];
    NSString *documentPath = [documentDir stringByAppendingPathComponent:@"latest"];
    const char *filename = [documentPath cStringUsingEncoding:NSUTF8StringEncoding];
    NSString *solutionPath = [documentDir stringByAppendingPathComponent:@"latest_solution"];
    const char *outname = [solutionPath cStringUsingEncoding:NSUTF8StringEncoding];
    if(replay || capture) _databuffer->filename = filename;
    else _databuffer->filename = NULL;
    _databuffer->size = 256 * 1024 * 1024;
    
    struct plugin mbp = mapbuffer_open(_databuffer);
    plugins_register(mbp);
    struct plugin disp = dispatch_init(_databuffer_dispatch);
    plugins_register(disp);
    if(filter) _cor_setup = new filter_setup(_databuffer_dispatch, outname);
    else _cor_setup = NULL;
}

- (void)teardownPlugins
{
    delete _databuffer_dispatch;
    delete _databuffer;
    if (_cor_setup) delete _cor_setup;
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

- (void)receiveVideoFrame:(unsigned char*)pixel withWidth:(uint32_t)width withHeight:(uint32_t)height withTimestamp:(CMTime)timestamp
{
    if (isPluginsStarted)
    {
        packet_t *buf = mapbuffer_alloc(_databuffer, packet_camera, width*height + 16); // 16 bytes for pgm header
    
        sprintf((char *)buf->data, "P5 %4d %3d %d\n", width, height, 255);
        unsigned char *outbase = buf->data + 16;
        memcpy(outbase, pixel, width*height);
        
        uint64_t time_us = timestamp.value / (timestamp.timescale / 1000000.);
        mapbuffer_enqueue(_databuffer, buf, time_us);
        if(_cor_setup) {
            NSLog(@"current filter position is: ");
            _cor_setup->sfm.s.T.v.print();
        }
    }
}

- (void)receiveAccelerometerData:(uint64_t)timestamp withX:(long)x withY:(long)y withZ:(long)z
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

- (void)receiveGyroData:(uint64_t)timestamp withX:(long)x withY:(long)y withZ:(long)z
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
