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



#import <Foundation/Foundation.h>

/*!
 * This is just a normalized object representation of a request sent to the native app from Javascript. It's really just
 * a way for us to pass this request information to our native code that is going to respond to the request.
 */
@interface MPNativeAction : NSObject {
    NSString *mAction;
    NSString *mMethod;
    NSDictionary *mParams;
}

@property(copy) NSString *action;
@property(copy) NSString *method;
@property(strong) NSDictionary *params;

- (id)initWithAction:(NSString *)action method:(NSString *)method params:(NSDictionary *)params;

+ (id)objectWithAction:(NSString *)action method:(NSString *)method params:(NSDictionary *)params;


@end