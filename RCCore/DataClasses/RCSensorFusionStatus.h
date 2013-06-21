//
// Created by Ben Hirashima on 6/20/13.
// Copyright (c) 2013 RealityCap. All rights reserved.
//
// To change the template use AppCode | Preferences | File Templates.
//

@interface RCSensorFusionStatus : NSObject

@property (nonatomic, readonly) float initializationProgress;
@property (nonatomic, readonly) int statusCode;
@property (nonatomic, readonly, getter=isSteady) BOOL steady;

- (id) initWithProgress:(float)initializationProgress withStatusCode:(int)statusCode withIsSteady:(BOOL)isSteady;

@end