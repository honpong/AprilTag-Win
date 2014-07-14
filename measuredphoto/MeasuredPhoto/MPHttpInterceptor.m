
#import "MPHttpInterceptor.h"
#import "NSObject+SBJson.h"
#import "GTMNSDictionary+URLArguments.h"

static NSString *RNCachingURLHeader = @"X-RNCache";

@interface MPHttpInterceptor ()

@property (nonatomic, readwrite, strong) NSURLConnection *connection;
@property (nonatomic, readwrite, strong) NSMutableData *data;
@property (nonatomic, readwrite, strong) NSURLResponse *response;

@end

@implementation MPHttpInterceptor

+ (BOOL)canInitWithRequest:(NSURLRequest *)request
{
  // only handle http requests we haven't marked with our header.
  if ([[[request URL] scheme] isEqualToString:@"http"] &&
      ([request valueForHTTPHeaderField:RNCachingURLHeader] == nil)) {
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

    NSString *jsonString = [NSString stringWithFormat:@"{\"message\":\"%@\"}", [params objectForKey:@"message"]];
    NSData *jsonBody = [jsonString dataUsingEncoding:NSUTF8StringEncoding];
    NSHTTPURLResponse *response = [[NSHTTPURLResponse alloc] initWithURL:self.request.URL statusCode:200 HTTPVersion:@"HTTP/1.1" headerFields:nil];

    [[self client] URLProtocol:self didReceiveResponse:response cacheStoragePolicy:NSURLCacheStorageNotAllowed]; // we handle caching ourselves.
    [[self client] URLProtocol:self didLoadData:jsonBody];
    [[self client] URLProtocolDidFinishLoading:self];
}

- (void)stopLoading
{
  [[self connection] cancel];
}

@end
