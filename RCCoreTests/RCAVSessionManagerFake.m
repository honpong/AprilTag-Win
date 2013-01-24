//
//  RCAVSessionManagerStub.m
//  RCCore
//
//  Created by Ben Hirashima on 1/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCAVSessionManagerFake.h"

@implementation RCAVSessionManagerFake

@synthesize session, videoPreviewLayer;

- (id)init
{
    NSLog(@"RCAVSessionManagerFake init");
    
    self = [super init];
    
    if (self)
    {
        isRunning = NO;
    }
    
    return self;
}

- (void)setupAVSession
{
    NSLog(@"RCAVSessionManagerFake setupAVSession");
}

- (bool)addOutput:(AVCaptureVideoDataOutput*)output
{
    NSLog(@"RCAVSessionManagerFake addOutput");
    return true;
}

- (bool)startSession
{
    NSLog(@"RCAVSessionManagerFake startSession");
    isRunning = YES;
    return true;
}

- (void)endSession
{
    NSLog(@"RCAVSessionManagerFake endSession");
    isRunning = NO;
}

- (bool)isRunning
{
    NSLog(@"RCAVSessionManagerFake isRunning");
    return isRunning;
}

@end
