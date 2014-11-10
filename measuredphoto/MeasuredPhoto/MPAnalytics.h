//
//  MPAnalytics.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/15/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

@interface MPAnalytics : NSObject

+ (void) logEvent: (NSString*)eventName;
+ (void) logEvent: (NSString*)eventName withParameters: (NSDictionary*)params;
+ (void) logError: (NSString*) eventName message:(NSString*)message error:(NSError*)error;
+ (void) logError: (NSString*) eventName message:(NSString*)message exception:(NSException*)exception;
+ (void) startTimedEvent: (NSString*)eventName withParameters: (NSDictionary*)params;
+ (void) endTimedEvent: (NSString*)eventName;

@end
