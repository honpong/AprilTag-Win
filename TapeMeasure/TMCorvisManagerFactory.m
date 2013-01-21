//
//  TMCorVisManagerFactory.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 1/16/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "TMCorvisManagerFactory.h"

@interface TMCorvisManagerImpl : NSObject <TMCorvisManager>
{
    struct outbuffer _databuffer;
    bool isPluginsStarted;
}

- (void)startPlugins;
- (void)stopPlugins;
- (void)receiveVideoFrame:(unsigned char*)pixel withWidth:(uint32_t)width withHeight:(uint32_t)height withTimestamp:(CMTime)timestamp;
- (void)receiveAccelerometerData:(uint64_t)timestamp withX:(long)x withY:(long)y withZ:(long)z;
- (void)receiveGyroData:(uint64_t)timestamp withX:(long)x withY:(long)y withZ:(long)z;
@end

@implementation TMCorvisManagerImpl

- (id)init
{
    self = [super init];
    
    if (self)
    {
        NSLog(@"CovisManager init");
        
        isPluginsStarted = NO;
    }
    
    return self;
}

- (void)startPlugins
{
    NSLog(@"CovisManager.startPlugins");
    
    if (!isPluginsStarted)
    {
        NSArray  *documentDirList = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
        NSString *documentDir  = [documentDirList objectAtIndex:0];
        NSString *documentPath = [documentDir stringByAppendingPathComponent:@"latest"];
        const char *filename = [documentPath cStringUsingEncoding:NSUTF8StringEncoding];
        
        _databuffer.filename = filename;
        _databuffer.size = 32 * 1024 * 1024;
        
        struct plugin mbp = outbuffer_open(&(_databuffer));
        plugins_register(mbp);
        
        cor_time_init();
        plugins_start();
        
        isPluginsStarted = YES;
    }
}

- (void)stopPlugins
{
    NSLog(@"CovisManager.stopPlugins");
    
    if (isPluginsStarted)
    {
        isPluginsStarted = NO;
        outbuffer_close(&(_databuffer));
        plugins_stop();
    }
}

- (void)receiveVideoFrame:(unsigned char*)pixel withWidth:(uint32_t)width withHeight:(uint32_t)height withTimestamp:(CMTime)timestamp
{
    if (isPluginsStarted)
    {
        packet_t *buf = outbuffer_alloc(&(_databuffer), packet_camera, width*height + 16); // 16 bytes for pgm header
    
        sprintf((char *)buf->data, "P5 %4d %3d %d\n", width, height, 255);
        unsigned char *outbase = buf->data + 16;
        memcpy(outbase, pixel, width*height);
        
        uint64_t time_us = timestamp.value / (timestamp.timescale / 1000000.);
        outbuffer_enqueue(&(_databuffer), buf, time_us);
    }
}

//- (void)receiveVideoFrame:(CVPixelBufferRef) pixelBuffer withTimestamp:(CMTime)timestamp
//{
//    if (isPluginsStarted)
//    {
//        uint32_t width = CVPixelBufferGetWidth(pixelBuffer);
//        uint32_t height = CVPixelBufferGetHeight(pixelBuffer);
//        packet_t *buf = outbuffer_alloc(&_databuffer, packet_camera, width*height + 16); // 16 bytes for pgm header
//        
//        sprintf((char *)buf->data, "P5 %4d %3d %d\n", width, height, 255);
//        char *outbase = buf->data + 16;
//        CVPixelBufferLockBaseAddress(pixelBuffer, 0);
//        unsigned char *pixel = (unsigned char *)CVPixelBufferGetBaseAddressOfPlane(pixelBuffer,0);
//        memcpy(outbase, pixel, width*height);
//        
//        CVPixelBufferUnlockBaseAddress(pixelBuffer, 0);
//        uint64_t time_us = timestamp.value / (timestamp.timescale / 1000000.);
//        outbuffer_enqueue(&_databuffer, buf, time_us);
//    }
//}

- (void)receiveAccelerometerData:(uint64_t)timestamp withX:(long)x withY:(long)y withZ:(long)z
{
    if (isPluginsStarted)
    {
        packet_t *p = outbuffer_alloc(&(_databuffer), packet_accelerometer, 3*4);
        //ios gives acceleration in g-units, so multiply by standard gravity in m/s^2
        ((float*)p->data)[0] = x * 9.80665;
        ((float*)p->data)[1] = y * 9.80665;
        ((float*)p->data)[2] = z * 9.80665;
        outbuffer_enqueue(&(_databuffer), p, timestamp * 1000000);
    }
}

- (void)receiveGyroData:(uint64_t)timestamp withX:(long)x withY:(long)y withZ:(long)z
{
    if (isPluginsStarted)
    {
        packet_t *p = outbuffer_alloc(&(_databuffer), packet_gyroscope, 3*4);
        ((float*)p->data)[0] = x;
        ((float*)p->data)[1] = y;
        ((float*)p->data)[2] = z;
        outbuffer_enqueue(&(_databuffer), p, timestamp * 1000000);
    }
}

@end

@implementation TMCorvisManagerFactory

static id<TMCorvisManager> instance;

+ (id<TMCorvisManager>)getCorvisManagerInstance
{
    if (instance == nil)
    {
        instance = [[TMCorvisManagerImpl alloc] init];
    }
    
    return instance;
}

//for testing. you can set this factory to return a mock object.
+ (void)setCorvisManagerInstance:(id<TMCorvisManager>)mockObject
{
    instance = mockObject;
}

@end
