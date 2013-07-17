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

- (id) initWithProgress:(float)initializationProgress withStatusCode:(int)statusCode withIsSteady:(BOOL)isSteady
{
    if(self = [super init])
    {
        _initializationProgress = initializationProgress;
        _statusCode = statusCode;
        _steady = isSteady;
    }
    return self;
}

@end