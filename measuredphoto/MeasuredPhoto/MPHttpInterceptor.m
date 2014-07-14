
#import "MPHttpInterceptor.h"
#import "NSObject+SBJson.h"
#import "GTMNSDictionary+URLArguments.h"
#import "SBJsonWriter.h"

static NSString *RNCachingURLHeader = @"X-RNCache";

static id<MPHttpInterceptorDelegate> delegate;

@interface MPHttpInterceptor ()

@property (nonatomic, readwrite, strong) NSURLConnection *connection;
@property (nonatomic, readwrite, strong) NSMutableData *data;
@property (nonatomic, readwrite, strong) NSURLResponse *response;

@end

@implementation MPHttpInterceptor

+ (id<MPHttpInterceptorDelegate>) delegate
{
    return delegate;
}

+ (void) setDelegate:(id<MPHttpInterceptorDelegate>)delegate_
{
    delegate = delegate_;
}

+ (BOOL)canInitWithRequest:(NSURLRequest *)request
{
  // only handle http requests we haven't marked with our header.
  if ([[[request URL] scheme] isEqualToString:@"http"] && ([request valueForHTTPHeaderField:RNCachingURLHeader] == nil))
  {
    return YES;
  }
  return NO;
}

+ (NSURLRequest *)canonicalRequestForRequest:(NSURLRequest *)request
{
  return request;
}

- (void)startLoading
{
    NSHTTPURLResponse *response = nil;
    NSData *jsonBody = nil;
    
    if (MPHttpInterceptor.delegate && [MPHttpInterceptor.delegate respondsToSelector:@selector(handleAction:error:)])
    {
        NSString *action = nil;
        if ([[self.request.URL pathComponents] count] > 1) { // use index 1 since index 0 is the '/'
            action = [[self.request.URL pathComponents] objectAtIndex:1];
        }
        NSString *query = [self.request.URL query];
        NSString *method = [self.request HTTPMethod];
        NSDictionary *params = nil;

        if ([method isEqualToString:@"POST"] || [method isEqualToString:@"PUT"]) {
            NSString *body = nil;
            body = [[NSString alloc] initWithData:[self.request HTTPBody] encoding:NSUTF8StringEncoding];
            
            if ([[self.request valueForHTTPHeaderField:@"content-type"] isEqualToString:@"application/json"])
            {
                params = [body JSONValue];
            }
            else
            {
                params = [NSDictionary gtm_dictionaryWithHttpArgumentsString:body];
            }
        }
        else
        {
            params = [NSDictionary gtm_dictionaryWithHttpArgumentsString:query];
        }

        MPNativeAction *nativeAction = [[MPNativeAction alloc] initWithAction:action method:method params:params];
        NSError *error1 = nil;
        NSMutableDictionary *result = [[MPHttpInterceptor.delegate handleAction:nativeAction error:&error1] mutableCopy];
        
        // if we got an error, put it in the json we'll pass back to javascript. TODO: send 500 status instead?
        if (error1)
        {
            [result setObject:@{ @"code" : [NSNumber numberWithInt:error1.code], @"message" : [error1 localizedDescription] } forKey:@"error"];
        }
        
        if (result)
        {
            NSString *jsonString = [[[SBJsonWriter alloc] init] stringWithObject:result];
            jsonBody = [jsonString dataUsingEncoding:NSUTF8StringEncoding];
            response = [[NSHTTPURLResponse alloc] initWithURL:self.request.URL statusCode:200 HTTPVersion:@"HTTP/1.1" headerFields:nil];
        }
        else
        {
            response = [[NSHTTPURLResponse alloc] initWithURL:self.request.URL statusCode:500 HTTPVersion:@"HTTP/1.1" headerFields:nil];
        }
    }
    else
    {
        response = [[NSHTTPURLResponse alloc] initWithURL:self.request.URL statusCode:500 HTTPVersion:@"HTTP/1.1" headerFields:nil];
    }
    
    [[self client] URLProtocol:self didReceiveResponse:response cacheStoragePolicy:NSURLCacheStorageNotAllowed];
    if (jsonBody) [[self client] URLProtocol:self didLoadData:jsonBody];
    [[self client] URLProtocolDidFinishLoading:self];
}

- (void)stopLoading
{
  [[self connection] cancel];
}

@end
