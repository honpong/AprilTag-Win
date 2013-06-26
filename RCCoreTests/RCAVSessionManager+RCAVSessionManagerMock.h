//
//  RCAVSessionManager+RCAVSessionManagerMock.h
//  RCCore
//
//  Created by Ben Hirashima on 6/26/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCAVSessionManager.h"

static id mockAVSessionManager;

@interface RCAVSessionManager (RCAVSessionManagerMock)

+ (RCAVSessionManager*) sharedInstance;
+ (void) setupMock;
+ (void) setupNiceMock;

@end
