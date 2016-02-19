//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

#import "RCSensorFusionStatus.h"
#import "NSError+RCDictionaryRepresentation.h"

@implementation RCSensorFusionStatus
{

}

- (id) initWithRunState:(RCSensorFusionRunState)runState withProgress:(float)progress withConfidence:(RCSensorFusionConfidence)confidence withError:(NSError*)error
{
    if(self = [super init])
    {
        _runState = runState;
        _progress = progress;
        _confidence = confidence;
        _error = error;
    }
    return self;
}

- (NSDictionary *)dictionaryRepresentation
{
    NSDictionary* dict;
    if (self.error)
    {
        dict = @{
                 @"runState": @(self.runState),
                 @"progress": @(self.progress),
                 @"confidence": @(self.confidence),
                 @"error": [self.error dictionaryRepresentation]
                 };
    }
    else
    {
        dict = @{
                 @"runState": @(self.runState),
                 @"progress": @(self.progress),
                 @"confidence": @(self.confidence)
                 };
    }
    return dict;
}

@end