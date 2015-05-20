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

#import <Foundation/Foundation.h>
#import "ARNativeAction.h"

@protocol RCHttpInterceptorDelegate <NSObject>
- (NSDictionary *)handleAction:(ARNativeAction *)action error:(NSError **)error;
@end

/** This class intercepts all HTTP/S requests to dummy.realitycap.com from the app, including from the UIWebView. */
@interface RCHttpInterceptor : NSURLProtocol

/** The delegate must belong to the class instead of the instance, because the system handles instantiation of this class. That means that only one object at a time may be a RCHttpInterceptorDelegate. */
+ (id<RCHttpInterceptorDelegate>) delegate;
+ (void) setDelegate:(id<RCHttpInterceptorDelegate>) delegate;

@end