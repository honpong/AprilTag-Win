//
//  RCHttpClientFactoryTests.m
//  RCCore
//
//  Created by Ben Hirashima on 2/28/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCHttpClientFactoryTests.h"
#import "RCHttpClientFactory.h"
#import "OCMock.h"
#import "RCUser.h"
#import "RCHTTPClient.h"

@implementation RCHttpClientFactoryTests

- (void)setUp
{
    [super setUp];
    
    // Set-up code here.
}

- (void)tearDown
{
    [RCHttpClientFactory setInstance:nil];
    
    [super tearDown];
}

- (void)testReturnsSameInstance
{
    [RCHttpClientFactory initWithBaseUrl:@"https://internal.realitycap.com/" withAcceptHeader:@"application/vnd.realitycap.json; version=1.0" withApiVersion:1];
    
    RCHTTPClient *client1 = [RCHttpClientFactory getInstance];
    RCHTTPClient *client2 = [RCHttpClientFactory getInstance];
    
    STAssertEqualObjects(client1, client2, @"Get instance failed to return the same instance");
}

- (void)testSetInstance
{
    RCHTTPClient *client1 = [OCMockObject mockForClass:[RCHTTPClient class]];
    
    [RCHttpClientFactory setInstance:client1];
    
    RCHTTPClient *client2 = [RCHttpClientFactory getInstance];
    
    STAssertEqualObjects(client1, client2, @"Get instance failed to return the same instance after set instance was called");
}

- (void)testGetInstanceWithoutInitFails
{
    RCHTTPClient *instance = [RCHttpClientFactory getInstance];
    STAssertNil(instance, @"Instance requested without being setup first");
}
@end
