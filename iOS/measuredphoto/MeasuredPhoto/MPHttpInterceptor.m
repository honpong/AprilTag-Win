
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
    NSData *responseBody = nil;
    
    if (MPHttpInterceptor.delegate && [MPHttpInterceptor.delegate respondsToSelector:@selector(handleAction:error:)])
    {
        MPNativeAction *nativeAction = [MPNativeAction nativeActionWithRequest:self.request];
        NSError *error = nil;
        NSMutableDictionary *result = [[MPHttpInterceptor.delegate handleAction:nativeAction error:&error] mutableCopy];
        
        if (error)
        {
            // error code contains HTTP status code
            response = [[NSHTTPURLResponse alloc] initWithURL:self.request.URL statusCode:error.code HTTPVersion:@"HTTP/1.1" headerFields:nil];
            responseBody = [error.localizedDescription dataUsingEncoding:NSUTF8StringEncoding];
        }
        else if (result)
        {
            NSString *jsonString = [[[SBJsonWriter alloc] init] stringWithObject:result];
            responseBody = [jsonString dataUsingEncoding:NSUTF8StringEncoding];
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
    if (responseBody) [[self client] URLProtocol:self didLoadData:responseBody];
    [[self client] URLProtocolDidFinishLoading:self];
}

- (void)stopLoading
{
  [[self connection] cancel];
}

@end
