//
//    Copyright (c) 2014, RealityCap, Inc.
//    All rights reserved.
//
//    BSD License
//
//    Redistribution and use in source and binary forms, with or without
//    modification, are permitted provided that the following conditions are met:
//    * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//    * Neither the name of RealityCap, Inc., nor the
//    names of its contributors may be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
//    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//    DISCLAIMED. IN NO EVENT SHALL RealityCap, Inc. BE LIABLE FOR ANY
//    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#import "RCHttpInterceptor.h"
#import "NSObject+SBJson.h"
#import "GTMNSDictionary+URLArguments.h"
#import "SBJsonWriter.h"

static id<RCHttpInterceptorDelegate> delegate;

@interface RCHttpInterceptor ()

@property (nonatomic, readwrite, strong) NSURLConnection *connection;
@property (nonatomic, readwrite, strong) NSMutableData *data;
@property (nonatomic, readwrite, strong) NSURLResponse *response;

@end

@implementation RCHttpInterceptor

+ (id<RCHttpInterceptorDelegate>) delegate
{
    return delegate;
}

+ (void) setDelegate:(id<RCHttpInterceptorDelegate>)delegate_
{
    delegate = delegate_;
}

+ (BOOL)canInitWithRequest:(NSURLRequest *)request
{
    if ([request.URL.host isEqualToString:@"dummy.realitycap.com"])
        return YES;
    else
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
    
    if (RCHttpInterceptor.delegate && [RCHttpInterceptor.delegate respondsToSelector:@selector(handleAction:error:)])
    {
        ARNativeAction *nativeAction = [ARNativeAction nativeActionWithRequest:self.request];
        NSError *error = nil;
        NSMutableDictionary *result = [[RCHttpInterceptor.delegate handleAction:nativeAction error:&error] mutableCopy];
        
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
