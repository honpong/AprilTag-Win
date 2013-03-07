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

@protocol TMServerOps <NSObject>

- (void) loginOrCreateAccount: (void (^)())completionBlock;
- (void) createAnonAccount: (void (^)())completionBlock;
- (void) login: (void (^)())completionBlock;
- (void) syncWithServer: (void (^)())completionBlock;
- (void) logout: (void (^)())completionBlock;

@end

@interface TMServerOpsFactory : NSObject

+ (id<TMServerOps>) getInstance;
+ (void) setInstance: (id<TMServerOps>)mockObject;

@end