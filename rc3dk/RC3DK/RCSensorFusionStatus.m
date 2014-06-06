//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCSensorFusionStatus.h"

@implementation RCSensorFusionStatus
{

}

- (id) initWithState:(RCSensorFusionState)state withProgress:(float)calibrationProgress withErrorCode:(int)errorCode withIsSteady:(BOOL)isSteady
{
    if(self = [super init])
    {
        _state = state;
        _calibrationProgress = calibrationProgress;
        _errorCode = errorCode;
        _steady = isSteady;
    }
    return self;
}

@end