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

- (id) initWithProgress:(float)calibrationProgress withStatusCode:(int)statusCode withIsSteady:(BOOL)isSteady
{
    if(self = [super init])
    {
        _calibrationProgress = calibrationProgress;
        _statusCode = statusCode;
        _steady = isSteady;
    }
    return self;
}

@end