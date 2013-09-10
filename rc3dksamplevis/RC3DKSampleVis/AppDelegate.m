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
            bool good = stddev / depth < .02;
            [_glview observeFeatureWithId:fid x:x y:y z:z lastSeen:time good:good];
        }
        NSDictionary * transformation = [dict objectForKey:@"transformation"];
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
        connectedSocket = newSocket;
        NSLog(@"Accepted a new connection");
        [_glview reset];
    }
    else
    {
        NSLog(@"You tried to connect again, for shame");
    }
    [newSocket readDataToLength:headerLength withTimeout:-1 tag:TAG_FIXED_LENGTH_HEADER];
}

- (void)socketDidDisconnect:(GCDAsyncSocket *)sock withError:(NSError *)err
{
    [_glview setViewpoint:RCViewpointAnimating];
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

- (IBAction)handleViewChange:(id)sender
{
    [_topDownViewMenuItem setState:NSOffState];
    [_sideViewMenuItem setState:NSOffState];
    [_animateViewMenuItem setState:NSOffState];

    if(sender == _topDownViewMenuItem)
        [_glview setViewpoint:RCViewpointTopDown];
    else if (sender == _sideViewMenuItem)
        [_glview setViewpoint:RCViewpointSide];
    else //if (sender == _animateViewMenuItem)
        [_glview setViewpoint:RCViewpointAnimating];
    [sender setState:NSOnState];
}

- (IBAction)handleFeatureChange:(id)sender
{
    [_allFeaturesMenuItem setState:NSOffState];
    [_filterFeaturesMenuItem setState:NSOffState];

    if(sender == _allFeaturesMenuItem)
        [_glview setFeatureFilter:RCFeatureFilterShowAll];
    else
        [_glview setFeatureFilter:RCFeatureFilterShowGood];
    [sender setState:NSOnState];
}

@end
