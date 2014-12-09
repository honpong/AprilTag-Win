//
//  NSString+RCString.h
//  RCCore
//
//  Created by Ben Hirashima on 2/26/13.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface NSString (RCString)

- (BOOL) containsString:(NSString*)search;
- (BOOL) beginsWithString:(NSString*)search;
- (BOOL) endsWithString:(NSString*)search;

@end
