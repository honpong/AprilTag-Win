//
//  NSLayoutConstraint+RCAutoLayoutDebuggin.h
//  DistanceLabelTest
//
//  Created by Ben Hirashima on 10/2/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface NSLayoutConstraint (RCAutoLayoutDebugging)

#ifdef DEBUG
- (NSString *)description;
#endif

@end
