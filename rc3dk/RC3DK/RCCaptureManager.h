//
//  CaptureController.h
//  RCCapture
//
//  Created by Brian on 10/3/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

#import "RCSensorFusion.h"

@interface RCCaptureManager : NSObject <RCSensorDataDelegate>

- (void)startCaptureWithPath:(NSString *)path;
- (void)stopCapture;

@end