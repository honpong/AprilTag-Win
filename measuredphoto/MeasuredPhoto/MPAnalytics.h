//
//  TMAnalytics.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/15/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//


@interface MPAnalytics : NSObject

+ (void) logEventWithCategory:(NSString*)category withAction:(NSString*)action withLabel:(NSString*)label withValue:(NSNumber*)value;
+ (void) logError:(NSString*)errorType withMessage:(NSString*)errorMessage;
+ (void) logError:(NSString*)errorType withError:(NSError*)error;
+ (void) logError:(NSString*)errorType withException: (NSException*)exception;

@end
