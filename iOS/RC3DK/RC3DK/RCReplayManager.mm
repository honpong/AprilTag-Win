//
//  ReplayController.m
//  RCCapture
//
//  Created by Brian on 2/18/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#import "RCReplayManager.h"

#include "replay.h"

@interface RCReplayManager () {
    replay rp;
    dispatch_queue_t queue;
}
@end

@implementation RCReplayManager

@synthesize delegate;

+ (id) sharedInstance
{
    static RCReplayManager *instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [RCReplayManager new];
    });
    return instance;
}

- (id) init
{
    self = [super init];
    if (self) {
        queue = dispatch_queue_create("com.realitycap.replay", DISPATCH_QUEUE_SERIAL);
    }
    return self;
}

- (void)cleanup
{
    NSLog(@"Straight-line length is %.2f cm, total path length %.2f cm\n", rp.get_length(), rp.get_path_length());
    fprintf(stderr, "Dispatched %llu packets %.2f Mbytes\n", rp.get_packets_dispatched(), rp.get_bytes_dispatched()/1.e6);

    if(delegate && [delegate respondsToSelector:@selector(replayFinishedWithLength:withPathLength:)])
        dispatch_async(dispatch_get_main_queue(), ^{
            [delegate replayFinishedWithLength:rp.get_length() withPathLength:rp.get_path_length()];
        });

}

- (void)startReplay
{
    dispatch_async(queue, ^(void) {
        rp.start();
        [self cleanup];
    });
}

- (void)setupWithPath:(NSString *)path withRealtime:(BOOL)realtime withWidth:(int)_width withHeight:(int)_height withFramerate:(int)_framerate
{
    NSLog(@"Setup replay with %@", path);

    /*
     width = _width;
     height = _height;
     framerate = _framerate;
     */
    __weak typeof(self) weakself = self;
    std::function<void (float)> callback = [weakself](float progress)
    {
        if(weakself.delegate &&
           [weakself.delegate respondsToSelector:@selector(replayProgress:)]) {
            dispatch_async(dispatch_get_main_queue(), ^{
                [weakself.delegate replayProgress:progress];
            });
        }
    };

    rp.set_progress_callback(callback);
    if(realtime)
        rp.enable_realtime();
    rp.open([path UTF8String]);
}

- (void)stopReplay
{
    rp.stop();
}

@end
