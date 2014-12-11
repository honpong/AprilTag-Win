
//  Copyright (c) 2014 Caterpillar. All rights reserved.

#import <Foundation/Foundation.h>
#import "CATNativeAction.h"

@protocol CATHttpInterceptorDelegate <NSObject>
- (NSDictionary *)handleAction:(CATNativeAction *)action error:(NSError **)error;
@end

/** This class intercepts all HTTP requests from the app, including from the UIWebView. */
@interface CATHttpInterceptor : NSURLProtocol

/** The delegate must belong to the class instead of the instance, because the system handles instantiation of this class. That means that only one object at a time may be a CATHttpInterceptorDelegate. */
+ (id<CATHttpInterceptorDelegate>) delegate;
+ (void) setDelegate:(id<CATHttpInterceptorDelegate>) delegate;

@end