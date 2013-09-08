//
//  RCSensorFusion+RCSensorFusionMock.h
//  RCCore
//
//  Created by Ben Hirashima on 6/27/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCSensorFusion.h"
#import "RCMockSingleton.h"

@interface RCSensorFusion (RCSensorFusionMock) <RCMockSingleton>

+ (RCSensorFusion*) sharedInstance;

@end
