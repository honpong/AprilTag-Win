//
//  RCAVSessionManager+RCAVSessionManagerMock.h
//  RCCore
//
//  Created by Ben Hirashima on 6/26/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCAVSessionManager.h"
#import "RCMockSingleton.h"

@interface RCAVSessionManager (RCAVSessionManagerMock) <RCMockSingleton>

+ (RCAVSessionManager*) sharedInstance;

@end
