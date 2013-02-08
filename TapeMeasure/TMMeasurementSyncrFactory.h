//
//  TMMeasurementSyncrFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 2/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCore/RCHttpClientFactory.h"
#import "TMMeasurement.h"
#import "TMDataManagerFactory.h"

@protocol TMMeasurementSyncr <NSObject>

- (void)fetchMeasurementChanges:(void (^)(NSArray*))successBlock onFailure:(void (^)(int))failureBlock;

@end

@interface TMMeasurementSyncrFactory : NSObject

+ (id<TMMeasurementSyncr>)getInstance;
+ (void)setInstance:(id<TMMeasurementSyncr>)mockObject;

@end