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
    if (![listenSocket acceptOnPort:58000 error:&error])
    {
        NSLog(@"Error listening on socket: %@", error);
    }

    /*
    GCDAsyncSocket * testSocket = [[GCDAsyncSocket alloc] initWithDelegate:self delegateQueue:dispatch_get_main_queue()];
    
    [testSocket connectToHost:@"localhost" onPort:58000 error:&error];
    if (error) {
        NSLog(@"Error doing the test connection: %@", error);
    }
     */
}


- (void)socket:(GCDAsyncSocket *)sock didConnectToHost:(NSString *)host port:(UInt16)port
{
    NSLog(@"Did connect");
    NSError * error = nil;
    NSDictionary * dict = [NSDictionary dictionaryWithObjectsAndKeys:@"one", @"id", @"two", @"otherid", nil];
    NSData * data = [NSJSONSerialization dataWithJSONObject:dict options:0 error:&error];
    uint64_t length = 0;
    length = [data length];
    NSData * header = [NSData dataWithBytes:&length length:headerLength];
    NSLog(@"Header %@", header);
    NSLog(@"Data %@", data);
    NSLog(@"len %llu", length);
    [sock writeData:header withTimeout:-1 tag:TAG_FIXED_LENGTH_HEADER];
    [sock writeData:data withTimeout:-1 tag:TAG_MESSAGE_BODY];
    [sock writeData:header withTimeout:-1 tag:TAG_FIXED_LENGTH_HEADER];
    [sock writeData:data withTimeout:-1 tag:TAG_MESSAGE_BODY];
}


-(uint64_t)parseHeader:(NSData *)data
{
    return 0;
}


- (void)socket:(GCDAsyncSocket *)sock didWriteDataWithTag:(long)tag
{
	NSLog(@"Did write with tag %ld", tag);
}


- (void)handleResponseBody:(NSData *)data
{
    NSLog(@"Parse body");
    NSError * error = nil;
    id parsed = [NSJSONSerialization JSONObjectWithData:data options:0 error:&error];

    if(error) {
        NSLog(@"JSON Error %@", error);
    }
    
    if (parsed == nil) {
        // Got nothing back, probably an error
    }
    else if ([parsed isKindOfClass: [NSArray class]])
    {
        NSArray * arr = parsed;
        NSLog(@"Got an array %@", arr);
    }
    else {
        NSDictionary * dict = parsed;
        NSLog(@"Got a dictionary %@", dict);
    }
    
    return;
}

- (void)socket:(GCDAsyncSocket *)sender didReadData:(NSData *)data withTag:(long)tag
{
    NSLog(@"Read some data %ld\n", [data length]);
    if (tag == TAG_FIXED_LENGTH_HEADER)
    {
        NSLog(@"Parse header");
        NSLog(@"Data is %@", data);
        NSLog(@"Data len %lu", [data length]);
        uint64_t bodyLength = *((uint64_t *)[data bytes]);
        bodyLength = CFSwapInt64BigToHost(bodyLength);
        NSLog(@"body length %llu", bodyLength);
        
        NSLog(@"Listening again");
        [sender readDataToLength:bodyLength withTimeout:-1 tag:TAG_MESSAGE_BODY];
    }
    else if (tag == TAG_MESSAGE_BODY)
    {
        // Process the response
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
@end
