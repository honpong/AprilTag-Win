//
//  NSString+RCString.h
//  RCCore
//
//  Created by Ben Hirashima on 2/26/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSString (RCString)

- (BOOL) containsString:(NSString*)search;
- (BOOL) beginsWithString:(NSString*)search;
- (BOOL) endsWithString:(NSString*)search;

@end
