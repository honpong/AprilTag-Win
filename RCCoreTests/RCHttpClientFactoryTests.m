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
    [RCHttpClientFactory initWithBaseUrl:@"https://internal.realitycap.com/" withAcceptHeader:@"application/vnd.realitycap.json; version=1.0"];
    
    AFHTTPClient *client1 = [RCHttpClientFactory getInstance];
    AFHTTPClient *client2 = [RCHttpClientFactory getInstance];
    
    STAssertEqualObjects(client1, client2, @"Get instance failed to return the same instance");
}

- (void)testSetInstance
{
    AFHTTPClient *client1 = [OCMockObject mockForClass:[AFHTTPClient class]];
    
    [RCHttpClientFactory setInstance:client1];
    
    AFHTTPClient *client2 = [RCHttpClientFactory getInstance];
    
    STAssertEqualObjects(client1, client2, @"Get instance failed to return the same instance after set instance was called");
}

- (void)testGetInstanceWithoutInitFails
{
    AFHTTPClient *instance = [RCHttpClientFactory getInstance];
    STAssertNil(instance, @"Instance requested without being setup first");
}
@end
