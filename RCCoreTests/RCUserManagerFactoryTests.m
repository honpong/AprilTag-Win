//
//  RCUserManagerFactoryTests.m
//  RCCore
//
//  Created by Ben Hirashima on 2/5/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCUserManagerFactoryTests.h"
#import "RCUserManagerFactory.h"

@implementation RCUserManagerFactoryTests

- (void)setUp
{
    [super setUp];
    
    // Set-up code here.
}

- (void)tearDown
{
    [RCUserManagerFactory setInstance:nil];
    
    [super tearDown];
}

- (void)testLogin
{
    id<RCUserManager> userMan = [RCUserManagerFactory getInstance];
    
    [userMan loginWithUsername:@"ben"
                  withPassword:@"ben"
                     onSuccess:^()
                     {
                         
                     }
                     onFailure:^(NSString *errorMsg)
                     {
                         STAssertTrue(false, @"FAIL");
                     }
     ];
}

@end
