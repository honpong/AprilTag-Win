//
//  ConnectionManager.m
//  RC3DKSampleApp
//
//  Created by Brian on 9/9/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCConnectionManager.h"

#import "GCDAsyncSocket.h"
#import <CoreFoundation/CoreFoundation.h>

#define TAG_FIXED_LENGTH_HEADER 0
#define TAG_MESSAGE_BODY 1
#define headerLength 8

@implementation RCConnectionManager
{
    GCDAsyncSocket* connectionSocket;
    NSNetServiceBrowser* netServiceBrowser;
    NSNetService *serverService;
    NSMutableArray *serverAddresses;
}

+ (RCConnectionManager *) sharedInstance
{
    static RCConnectionManager* instance = nil;
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        instance = [RCConnectionManager new];
    });
    return instance;
}


- (void)startSearch
{
    netServiceBrowser = [[NSNetServiceBrowser alloc] init];

    [netServiceBrowser setDelegate:self];
    [netServiceBrowser searchForServicesOfType:@"_RC3DKSampleVis._tcp." inDomain:@""];
}

- (void)netServiceBrowser:(NSNetServiceBrowser *)sender didNotSearch:(NSDictionary *)errorInfo
{
    NSLog(@"DidNotSearch: %@", errorInfo);
}

- (void)netServiceBrowser:(NSNetServiceBrowser *)sender
           didFindService:(NSNetService *)netService
               moreComing:(BOOL)moreServicesComing
{
    NSLog(@"DidFindService: %@", [netService name]);

    // Connect to the first service we find
    if (serverService == nil)
    {
        NSLog(@"Resolving...");

        serverService = netService;

        [serverService setDelegate:self];
        [serverService resolveWithTimeout:5.0];
    }
}

- (void)netServiceBrowser:(NSNetServiceBrowser *)sender
         didRemoveService:(NSNetService *)netService
               moreComing:(BOOL)moreServicesComing
{
    NSLog(@"DidRemoveService: %@", [netService name]);
}

- (void)netServiceBrowserDidStopSearch:(NSNetServiceBrowser *)sender
{
    NSLog(@"DidStopSearch");
}

- (void)netService:(NSNetService *)sender didNotResolve:(NSDictionary *)errorDict
{
    NSLog(@"DidNotResolve");
}

- (void)netServiceDidResolveAddress:(NSNetService *)sender
{
    NSLog(@"DidResolve: %@", [sender addresses]);

    if (serverAddresses == nil)
    {
        serverAddresses = [[sender addresses] mutableCopy];
    }

    if (connectionSocket == nil)
    {
        connectionSocket = [[GCDAsyncSocket alloc] initWithDelegate:self delegateQueue:dispatch_get_main_queue()];
    }
}

- (void)socket:(GCDAsyncSocket *)sock didConnectToHost:(NSString *)host port:(UInt16)port
{
    NSLog(@"Socket:DidConnectToHost: %@ Port: %hu", host, port);
}


- (void)socketDidDisconnect:(GCDAsyncSocket *)sock withError:(NSError *)err
{
    NSLog(@"Socket:DidDisconnect");
}

- (void)connect
{
    BOOL done = NO;

    while (!done && ([serverAddresses count] > 0))
    {
        NSData *addr;

        // Note: The serverAddresses array probably contains both IPv4 and IPv6 addresses.
        //
        // If your server is also using GCDAsyncSocket then you don't have to worry about it,
        // as the socket automatically handles both protocols for you transparently.
        addr = serverAddresses[0];
        [serverAddresses removeObjectAtIndex:0];

        NSLog(@"Attempting connection to %@", addr);

        NSError *err = nil;
        if ([connectionSocket connectToAddress:addr error:&err])
        {
            NSLog(@"Connected");
            done = YES;
        }
        else
        {
            NSLog(@"Unable to connect: %@", err);
        }

    }

    if (!done)
    {
        NSLog(@"Unable to connect to any resolved address");
    }
}

- (void)disconnect
{
    if([connectionSocket isConnected])
        [connectionSocket disconnectAfterWriting];
    serverService = nil;
    serverAddresses = nil;
}

- (BOOL) isConnected
{
    return [connectionSocket isConnected];
}

- (void)sendPacket:(NSDictionary *)packet
{
    NSError * error = nil;
    NSData * data = [NSJSONSerialization dataWithJSONObject:packet options:0 error:&error];
    uint64_t length = 0;
    length = [data length];
    length = CFSwapInt64HostToBig(length);
    NSData * header = [NSData dataWithBytes:&length length:headerLength];
    [connectionSocket writeData:header withTimeout:-1 tag:TAG_FIXED_LENGTH_HEADER];
    [connectionSocket writeData:data withTimeout:-1 tag:TAG_MESSAGE_BODY];
}

@end
