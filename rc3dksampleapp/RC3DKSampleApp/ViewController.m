//
//  ViewController.m
//  RC3DKSampleApp
//
//  Created by Ben Hirashima on 7/19/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "ViewController.h"

#import "GCDAsyncSocket.h"
#import <CoreFoundation/CoreFoundation.h>

#define TAG_FIXED_LENGTH_HEADER 0
#define TAG_MESSAGE_BODY 1
#define headerLength 8

@implementation ViewController
{
    AVSessionManager* sessionMan;
    MotionManager* motionMan;
    LocationManager* locationMan;
    VideoManager* videoMan;
    RCSensorFusion* sensorFusion;
    bool isStarted;
    NSDate *startedAt;
    GCDAsyncSocket* visSocket;
    NSNetServiceBrowser* netServiceBrowser;
    NSNetService *serverService;
    NSMutableArray *serverAddresses;
    bool connected;
}
@synthesize startStopButton, distanceText;

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    //register to receive notifications of pause/resume events
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(teardown)
                                                 name:UIApplicationWillResignActiveNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(setup)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
    
    netServiceBrowser = [[NSNetServiceBrowser alloc] init];
	
    [netServiceBrowser setDelegate:self];
    [netServiceBrowser searchForServicesOfType:@"_RC3DKSampleVis._tcp." inDomain:@"local."];
}

- (void)setup
{
    sessionMan = [AVSessionManager sharedInstance];
    videoMan = [VideoManager sharedInstance];
    motionMan = [MotionManager sharedInstance];
    locationMan = [LocationManager sharedInstance];
    sensorFusion = [RCSensorFusion sharedInstance];
    sensorFusion.delegate = self; // Tells RCSensorFusion where to pass data to
    
    [videoMan setupWithSession:sessionMan.session]; // Can cause UI to lag if called on UI thread.
    
    [motionMan startMotionCapture]; // Start motion capture early
    [locationMan startLocationUpdates]; // Asynchronously gets the location and stores it
    [sensorFusion startInertialOnlyFusion];
    
    isStarted = false;
    connected = false;
    [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
}

- (void)teardown
{
    [motionMan stopMotionCapture];
    [sensorFusion stopSensorFusion];
}

- (void)startFullSensorFusion
{
    CLLocation *currentLocation = [locationMan getStoredLocation];
    [sensorFusion setLocation:currentLocation];
    
    [sessionMan startSession];
    [sensorFusion startProcessingVideo];
    [videoMan startVideoCapture];
    startedAt = [NSDate date];
}

- (void)stopFullSensorFusion
{
    [videoMan stopVideoCapture];
    [sensorFusion stopProcessingVideo];
    [sessionMan endSession];
}

- (void)sendPacket:(NSDictionary *)packet
{
    NSError * error = nil;
    NSData * data = [NSJSONSerialization dataWithJSONObject:packet options:0 error:&error];
    uint64_t length = 0;
    length = [data length];
    length = CFSwapInt64HostToBig(length);
    NSData * header = [NSData dataWithBytes:&length length:headerLength];
    [visSocket writeData:header withTimeout:-1 tag:TAG_FIXED_LENGTH_HEADER];
    [visSocket writeData:data withTimeout:-1 tag:TAG_MESSAGE_BODY];
}

- (void)sendUpdate:(RCSensorFusionData *)data
{
    NSTimeInterval time = -[startedAt timeIntervalSinceNow];
    NSMutableDictionary * packet = [[NSMutableDictionary alloc] initWithObjectsAndKeys:[NSNumber numberWithDouble:time],@"time", nil];
    NSMutableArray * features = [[NSMutableArray alloc] initWithCapacity:[data.featurePoints count]];
    for (id object in data.featurePoints)
    {
        RCFeaturePoint * p = object;
        if([p initialized])
        {
            //NSLog(@"%lld %f %f %f", p.id, p.worldPoint.x, p.worldPoint.y, p.worldPoint.z);
            NSDictionary * f = [p dictionaryRepresentation];
            [features addObject:f];
        }
    }
    [packet setObject:features forKey:@"features"];
    [self sendPacket:packet];
}

// RCSensorFusionDelegate delegate method. Called after each video frame is processed.
- (void)sensorFusionDidUpdate:(RCSensorFusionData *)data
{
    if([visSocket isConnected])
        [self sendUpdate:data];
    
    float distanceFromStartPoint = sqrt(data.transformation.translation.x * data.transformation.translation.x + data.transformation.translation.y * data.transformation.translation.y + data.transformation.translation.z * data.transformation.translation.z);
    distanceText.text = [NSString stringWithFormat:@"%0.3fm", distanceFromStartPoint];
}

// RCSensorFusionDelegate delegate method. Called when sensor fusion is in an error state.
- (void)sensorFusionError:(RCSensorFusionError *)error
{
    NSLog(@"ERROR: %@", error.debugDescription);
}

- (IBAction)startStopButtonTapped:(id)sender
{
    if (isStarted)
    {
        [self stopFullSensorFusion];
        [startStopButton setTitle:@"Start" forState:UIControlStateNormal];
    }
    else
    {
        [self startFullSensorFusion];
        [startStopButton setTitle:@"Stop" forState:UIControlStateNormal];
    }
    isStarted = !isStarted;
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

    if (visSocket == nil)
    {
        visSocket = [[GCDAsyncSocket alloc] initWithDelegate:self delegateQueue:dispatch_get_main_queue()];
		
        [self connectToNextAddress];
    }
}

- (void)connectToNextAddress
{
    BOOL done = NO;
	
    while (!done && ([serverAddresses count] > 0))
    {
        NSData *addr;
        
        // Note: The serverAddresses array probably contains both IPv4 and IPv6 addresses.
        //
        // If your server is also using GCDAsyncSocket then you don't have to worry about it,
        // as the socket automatically handles both protocols for you transparently.
        
        if (YES) // Iterate forwards
        {
            addr = [serverAddresses objectAtIndex:0];
            [serverAddresses removeObjectAtIndex:0];
        }
        else // Iterate backwards
        {
            addr = [serverAddresses lastObject];
            [serverAddresses removeLastObject];
        }
        
        NSLog(@"Attempting connection to %@", addr);
        
        NSError *err = nil;
        if ([visSocket connectToAddress:addr error:&err])
        {
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

- (void)socket:(GCDAsyncSocket *)sock didConnectToHost:(NSString *)host port:(UInt16)port
{
    NSLog(@"Socket:DidConnectToHost: %@ Port: %hu", host, port);
    
    connected = YES;
}

@end
