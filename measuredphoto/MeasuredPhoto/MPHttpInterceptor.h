
#import <Foundation/Foundation.h>
#import "MPNativeAction.h"

@protocol MPHttpInterceptorDelegate <NSObject>
- (NSDictionary *)handleAction:(MPNativeAction *)action error:(NSError **)error;
@end

/** This class intercepts all HTTP requests from the app, including from the UIWebView. */
@interface MPHttpInterceptor : NSURLProtocol

/** The delegate must belong to the class instead of the instance, because the system handles instantiation of this class. That means that only one object at a time may be a MPHttpInterceptorDelegate. */
+ (id<MPHttpInterceptorDelegate>) delegate;
+ (void) setDelegate:(id<MPHttpInterceptorDelegate>) delegate;

@end