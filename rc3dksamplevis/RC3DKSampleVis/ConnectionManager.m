//
//  ConnectionManager.m
//  RC3DKSampleVis
//
//  Created by Brian on 2/27/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#import "ConnectionManager.h"
#import "RCOpenGLView.h"
#import "GCDAsyncSocket.h"

#define TAG_FIXED_LENGTH_HEADER 0
#define TAG_MESSAGE_BODY 1
#define headerLength 8

@implementation ConnectionManager

@synthesize visDataDelegate, connectionManagerDelegate;

+ (ConnectionManager *) sharedInstance
{
    static ConnectionManager* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [ConnectionManager new];
    });
    return instance;
}

- (void)startListening
{
    NSLog(@"Start listening");
    listenSocket = [[GCDAsyncSocket alloc] initWithDelegate:self delegateQueue:dispatch_get_main_queue()];

    NSError *error = nil;

    if ([listenSocket acceptOnPort:0 error:&error])
    {
        UInt16 port = [listenSocket localPort];

        // Create and publish the bonjour service.
        netService = [[NSNetService alloc] initWithDomain:@""
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
        //NSLog(@"packet %@", dict);
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
            [visDataDelegate observeFeatureWithId:fid x:x y:y z:z lastSeen:time good:good];
        }
        NSDictionary * transformation = [dict objectForKey:@"transformation"];
        NSDictionary * translation = [transformation objectForKey:@"translation"];
        float x = [[translation objectForKey:@"v0"] floatValue];
        float y = [[translation objectForKey:@"v1"] floatValue];
        float z = [[translation objectForKey:@"v2"] floatValue];
        [visDataDelegate observePathWithTranslationX:x y:y z:z time:time];
        [visDataDelegate observeTime:time];
    }
    return;
}

- (void)socket:(GCDAsyncSocket *)sender didReadData:(NSData *)data withTag:(long)tag
{
    NSLog(@"Read data");
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
    NSLog(@"Did accept new socket");
    if (!connectedSocket)
    {
        connectedSocket = newSocket;
        NSLog(@"Accepted a new connection");
        [visDataDelegate reset];
    }
    else
    {
        NSLog(@"You tried to connect again, for shame");
    }
    [newSocket readDataToLength:headerLength withTimeout:-1 tag:TAG_FIXED_LENGTH_HEADER];
}

- (void)socketDidDisconnect:(GCDAsyncSocket *)sock withError:(NSError *)err
{
    NSLog(@"DidDisconnect");
    if([connectionManagerDelegate respondsToSelector:@selector(connectionManagerDidDisconnect)])
        [connectionManagerDelegate connectionManagerDidDisconnect];
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

@end
