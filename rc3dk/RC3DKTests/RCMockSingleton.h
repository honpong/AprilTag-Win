//
//  RCMockSingleton.h
//  RCCore
//
//  Created by Ben Hirashima on 6/26/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@protocol RCMockSingleton <NSObject>

+ (void) setupMock;
+ (void) setupNiceMock;
+ (void) tearDownMock;

@end
