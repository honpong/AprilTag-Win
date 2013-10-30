//
//  TMAnalytics.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 3/15/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import "GAI.h"

static NSString* kAnalyticsCategoryUser = @"User";
static NSString* kAnalyticsCategoryFeedback = @"Feedback";

@interface MPAnalytics : NSObject

+ (id<GAITracker>) getTracker;
+ (void) logEventWithCategory:(NSString*)category withAction:(NSString*)action withLabel:(NSString*)label withValue:(NSNumber*)value;
+ (void) logError:(NSString*)errorType withMessage:(NSString*)errorMessage;
+ (void) logError:(NSString*)errorType withError:(NSError*)error;
+ (void) logError:(NSString*)errorType withException: (NSException*)exception;
+ (void) dispatch;

@end
