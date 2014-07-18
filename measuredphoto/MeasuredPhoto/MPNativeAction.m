/*!
 * \file    NativeAction
 * \project 
 * \author  Andy Rifken 
 * \date    11/25/12.
 *
 * Copyright 2012 Andy Rifken

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
   LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
   WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 */



#import "MPNativeAction.h"
#import "NSObject+SBJson.h"
#import "GTMNSDictionary+URLArguments.h"
#import "SBJsonWriter.h"

@implementation MPNativeAction
@synthesize body = mBody;
@synthesize request = mRequest;

+ (MPNativeAction*) nativeActionWithRequest:(NSURLRequest*)request
{
    return [[MPNativeAction alloc] initWithRequest:request];
}

- (id) initWithRequest:(NSURLRequest*)request
{
    if (self = [super init])
    {
        mRequest = request;
        mBody = nil;
        mParams = nil;
        mAction = nil;
    }
    return self;
}

- (NSString *)action
{
    if (mAction == nil && [[self.request.URL pathComponents] count] > 1) // use index 1 since index 0 is the '/'
    {
        mAction = [[self.request.URL pathComponents] objectAtIndex:1];
    }
    return mAction;
}

- (NSString*) method
{
    return self.request.HTTPMethod;
}

- (NSString*) body
{
    if (mBody == nil)
    {
        mBody = [[NSString alloc] initWithData:[self.request HTTPBody] encoding:NSUTF8StringEncoding];
    }
    return mBody;
}

- (NSDictionary*) params
{
    if (mParams == nil)
    {
        mParams = [self parseParams];
    }
    return mParams;
}

- (NSDictionary*) parseParams
{
    NSDictionary* params = nil;
    
    if ([self.request.HTTPMethod isEqualToString:@"POST"] || [self.request.HTTPMethod isEqualToString:@"PUT"])
    {
        if ([[self.request valueForHTTPHeaderField:@"content-type"] isEqualToString:@"application/json"])
        {
            params = [self.body JSONValue];
        }
        else
        {
            params = [NSDictionary gtm_dictionaryWithHttpArgumentsString:self.body];
        }
    }
    else
    {
        params = [NSDictionary gtm_dictionaryWithHttpArgumentsString:self.request.URL.query];
    }
    
    return params;
}


@end