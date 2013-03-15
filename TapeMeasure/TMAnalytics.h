//
//  TMAnalytics.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/15/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "Flurry.h"

@interface TMAnalytics : NSObject

+ (void) logEvent: (NSString*)eventName;
+ (void) logError: (NSString*) eventName message:(NSString*)message error:(NSError*)error;
+ (void) logError: (NSString*) eventName message:(NSString*)message exception:(NSException*)exception;

@end
