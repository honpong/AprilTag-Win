//
//  TMServerOpsFactory.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/7/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "RCCore/RCUserManagerFactory.h"
#import "TMMeasurement+TMMeasurementSync.h"
#import "TMLocation+TMLocationSync.h"

#define JSON_KEY_FLAG @"flag"
#define JSON_KEY_BLOB @"blob"

//these could all be static methods, but they are easier to test this way
@protocol TMServerOps <NSObject>

- (void) createAnonAccount: (void (^)())successBlock onFailure: (void (^)())failureBlock;
- (void) login: (void (^)())successBlock onFailure: (void (^)(int statusCode))failureBlock;
- (void) syncWithServer: (void (^)(BOOL updated))successBlock onFailure: (void (^)())failureBlock;
- (void) logout: (void (^)())completionBlock;
- (void) postJsonData:(NSDictionary*)params onSuccess:(void (^)())successBlock onFailure:(void (^)(int statusCode))failureBlock;

@end

@interface TMServerOpsFactory : NSObject

+ (id<TMServerOps>) getInstance;
+ (void) setInstance: (id<TMServerOps>)mockObject;

@end