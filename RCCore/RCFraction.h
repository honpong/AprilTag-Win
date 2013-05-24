//
//  RCFraction.h
//  RCCore
//
//  Created by Ben Hirashima on 5/23/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface RCFraction : NSObject

@property float floatValue;
@property int nominator;
@property int denominator;

- (BOOL) isEqualToOne;
- (NSString*) getString;

+ (RCFraction*) fractionWithInches:(float)inches;

@end
