//
//  NSLayoutConstraint+RCAutoLayoutDebuggin.m
//  DistanceLabelTest
//
//  Created by Ben Hirashima on 10/2/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "NSLayoutConstraint+RCAutoLayoutDebugging.h"
#import "UIView+RCAutoLayoutDebugging.h"

@interface NSLayoutConstraint ()
- (NSString *)asciiArtDescription;
@end

@implementation NSLayoutConstraint (RCAutoLayoutDebugging)

#ifdef DEBUG
- (NSString *)description
{
    NSString *description = super.description;
    NSString *asciiArtDescription = self.asciiArtDescription;
    return [description stringByAppendingFormat:@" %@ (%@, %@)", asciiArtDescription, [self.firstItem autoLayoutNameTag], [self.secondItem autoLayoutNameTag]];
}
#endif

@end
