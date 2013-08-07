//
//  RCHttpClientFactoryTests.m
//  RCCore
//
//  Created by Ben Hirashima on 2/28/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCHttpClientTests.h"
#import "RCHTTPClient.h"
#import "OCMock.h"
#import "RCUser.h"
#import "RCHTTPClient.h"

@implementation RCHttpClientTests

- (void)setUp
{
    [super setUp];
    // Set-up code here.
}

- (void)tearDown
{
    [super tearDown];
}

- (void)testNotNilAfterInit
{
    [RCHTTPClient initWithBaseUrl:@"https://internal.realitycap.com/" withAcceptHeader:@"application/vnd.realitycap.json; version=1.0" withApiVersion:1];
    RCHTTPClient *instance = [RCHTTPClient sharedInstance];
    STAssertNotNil(instance, @"Instance was nil after init");
}

- (void)testReturnsSameInstance
{
    [RCHTTPClient initWithBaseUrl:@"https://internal.realitycap.com/" withAcceptHeader:@"application/vnd.realitycap.json; version=1.0" withApiVersion:1];
    
    RCHTTPClient *client1 = [RCHTTPClient sharedInstance];
    RCHTTPClient *client2 = [RCHTTPClient sharedInstance];
    
    STAssertEqualObjects(client1, client2, @"Get instance failed to return the same instance");
}

- (void)testGetInstanceWithoutInitFails
{
    RCHTTPClient *instance = [RCHTTPClient sharedInstance];
    STAssertNil(instance, @"Instance requested without being setup first");
}

@end
