//
//  NSObject+SupersequentImplementation.h
//  RCCore
//
//  Created by Ben Hirashima on 6/26/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

#define invokeSupersequentNoParameters() \
([self getImplementationOf:_cmd \
after:impOfCallingMethod(self, _cmd)]) \
(self, _cmd)

@interface NSObject (SupersequentImplementation)

- (IMP)getImplementationOf:(SEL)lookup after:(IMP)skip;
IMP impOfCallingMethod(id lookupObject, SEL selector);

@end
