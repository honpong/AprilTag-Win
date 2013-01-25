//
//  RCVideoCapManagerFactoryTests.m
//  RCCore
//
//  Created by Ben Hirashima on 1/24/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCVideoCapManagerFactoryTests.h"
#import "RCVideoCapManagerFactory.h"
#import <OCMock.h>


@implementation RCVideoCapManagerFactoryTests

- (void)setUp
{
    [super setUp];
    
    // Set-up code here.
}

- (void)tearDown
{
    
    
    [super tearDown];
}

- (void)testSetupAddsSessionOutput
{
    id<RCVideoCapManager> videoMan = [RCVideoCapManagerFactory getVideoCapManagerInstance];
    
    id mockSession = [OCMockObject mockForClass:[AVCaptureSession class]];
//    [[mockSession expect] addOutput:[OCMArg any]];
    
    [videoMan setupVideoCapWithSession:mockSession withCorvisManager:[RCCorvisManagerFactory getCorvisManagerInstance]];
    
    [mockSession verify];
}

@end
