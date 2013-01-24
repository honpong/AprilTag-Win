//
//  RCAVSessionManagerStub.h
//  RCCore
//
//  Created by Ben Hirashima on 1/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "RCAVSessionManagerFactory.h"

@interface RCAVSessionManagerFake : NSObject <RCAVSessionManager>
{
    bool isRunning;
}

@end
