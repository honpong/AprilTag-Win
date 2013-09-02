//
//  AppDelegate.m
//  RC3DKSampleVis
//
//  Created by Brian on 8/26/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import "AppDelegate.h"

#import "GCDAsyncSocket.h"

#import <CoreFoundation/CoreFoundation.h>

@implementation AppDelegate

#define TAG_FIXED_LENGTH_HEADER 0
#define TAG_MESSAGE_BODY 1
#define headerLength 8

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    // Insert code here to initialize your application
    listenSocket = [[GCDAsyncSocket alloc] initWithDelegate:self delegateQueue:dispatch_get_main_queue()];
    
    NSError *error = nil;

    if ([listenSocket acceptOnPort:0 error:&error])
    {
        UInt16 port = [listenSocket localPort];

        // Create and publish the bonjour service.
        netService = [[NSNetService alloc] initWithDomain:@"local."
                                                     type:@"_RC3DKSampleVis._tcp."
                                                     name:@""
                                                     port:port];

        [netService setDelegate:self];
        [netService publish];
    }
    else
    {
        NSLog(@"Error listening on socket: %@", error);
    }

}

- (void)handleResponseBody:(NSData *)data
{
    //NSLog(@"Parse body");
    NSError * error = nil;
    id parsed = [NSJSONSerialization JSONObjectWithData:data options:0 error:&error];

    if(error)
        NSLog(@"JSON Error %@", error);

    if (parsed == nil)
    {
        // Got nothing back, probably an error
    }
    else if ([parsed isKindOfClass: [NSArray class]])
    {
        NSArray * arr = parsed;
        NSLog(@"Got an array, was expecting a dictionary %@", arr);
    }
    else {
        NSDictionary * dict = parsed;
        //NSLog(@"Got a dictionary %@", dict);
        NSDictionary * features = [dict objectForKey:@"features"];
        float time = [[dict objectForKey:@"time"] floatValue];

        for (id obj in features)
        {
            NSDictionary * f = obj;
            uint64_t fid = [[f objectForKey:@"id"] unsignedLongLongValue];
            NSDictionary * worldPoint = [f objectForKey:@"worldPoint"];
            float x = [[worldPoint objectForKey:@"v0"] floatValue];
            float y = [[worldPoint objectForKey:@"v1"] floatValue];
            float z = [[worldPoint objectForKey:@"v2"] floatValue];
            float depth = [[[f objectForKey:@"originalDepth"] objectForKey:@"scalar"] floatValue];
            float stddev = [[[f objectForKey:@"originalDepth"] objectForKey:@"standardDeviation"] floatValue];
            bool good = stddev / depth < .1;
            [_glview observeFeatureWithId:fid x:x y:y z:z lastSeen:time good:good];
        }
        NSDictionary * transformation = [dict objectForKey:@"transformation"];
        //NSDictionary * rotation = [transformation objectForKey:@"rotation"];
        NSDictionary * translation = [transformation objectForKey:@"translation"];
        float x = [[translation objectForKey:@"v0"] floatValue];
        float y = [[translation objectForKey:@"v1"] floatValue];
        float z = [[translation objectForKey:@"v2"] floatValue];
        [_glview observePathWithTranslationX:x y:y z:z time:time];
        [_glview drawForTime:time];
    }
    return;
}

- (void)socket:(GCDAsyncSocket *)sender didReadData:(NSData *)data withTag:(long)tag
{
    if (tag == TAG_FIXED_LENGTH_HEADER)
    {
        uint64_t bodyLength = *((uint64_t *)[data bytes]);
        bodyLength = CFSwapInt64BigToHost(bodyLength);

        // Start reading the next response
        [sender readDataToLength:bodyLength withTimeout:-1 tag:TAG_MESSAGE_BODY];
    }
    else if (tag == TAG_MESSAGE_BODY)
    {
        [self handleResponseBody:data];

        // Start reading the next response
        [sender readDataToLength:headerLength withTimeout:-1 tag:TAG_FIXED_LENGTH_HEADER];
    }
}

- (void)socket:(GCDAsyncSocket *)sender didAcceptNewSocket:(GCDAsyncSocket *)newSocket
{
    if (!connectedSocket)
    {
        NSLog(@"Connected a new socket");
        connectedSocket = newSocket;
        [_glview reset];
    }
    else
    {
        NSLog(@"You tried to connect again, for shame");
    }
    // The "sender" parameter is the listenSocket we created.
    // The "newSocket" is a new instance of GCDAsyncSocket.
    // It represents the accepted incoming client connection.

    // Do server stuff with newSocket...
    NSLog(@"Asking to read data");
    [newSocket readDataToLength:headerLength withTimeout:-1 tag:TAG_FIXED_LENGTH_HEADER];
}

- (void)socketDidDisconnect:(GCDAsyncSocket *)sock withError:(NSError *)err
{
    if(err)
        NSLog(@"Disconnected with error %@", err);
    else
        NSLog(@"Disconnected");
    connectedSocket = nil;
}


- (void)netServiceDidPublish:(NSNetService *)ns
{
    NSLog(@"Bonjour Service Published: domain(%@) type(%@) name(%@) port(%i)",
          [ns domain], [ns type], [ns name], (int)[ns port]);
}

- (void)netService:(NSNetService *)ns didNotPublish:(NSDictionary *)errorDict
{
    // Note: This method in invoked on our bonjour thread.
    NSLog(@"Failed to Publish Service: domain(%@) type(%@) name(%@) - %@",
          [ns domain], [ns type], [ns name], errorDict);
}

- (IBAction)handleViewMenu:(id)sender
{
    NSMenuItem * clicked = sender;
    if(clicked == _topDownViewMenuItem || clicked == _sideViewMenuItem || clicked == _cameraViewMenuItem) {
        [_topDownViewMenuItem setState:NSOffState];
        [_sideViewMenuItem setState:NSOffState];
        [_cameraViewMenuItem setState:NSOffState];

        if(clicked == _topDownViewMenuItem)
            [_glview setViewpoint:RCViewpointTopDown];
        else if (clicked == _sideViewMenuItem)
            [_glview setViewpoint:RCViewpointSide];
        else
            [_glview setViewpoint:RCViewpointDeviceView];
        [clicked setState:NSOnState];
    }

    if(clicked == _allFeaturesMenuItem || clicked == _filterFeaturesMenuItem) {
        [_allFeaturesMenuItem setState:NSOffState];
        [_filterFeaturesMenuItem setState:NSOffState];

        if(clicked == _allFeaturesMenuItem)
            [_glview setFeatureFilter:RCFeatureFilterShowAll];
        else
            [_glview setFeatureFilter:RCFeatureFilterShowGood];
        [clicked setState:NSOnState];
    }
}

@end
