//
//  CaptureController.h
//  RCCapture
//
//  Created by Brian on 10/3/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

#import "RCCaptureFile.h"

@protocol RCCaptureManagerDelegate <NSObject>

- (void) captureDidStop;
@optional
- (void) captureDidStart;

@end

@interface RCCaptureManager : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>

@property (weak, nonatomic) id<RCCaptureManagerDelegate> delegate;

- (void)startCaptureWithPath:(NSString *)path withDelegate:(id<RCCaptureManagerDelegate>)captureDelegate;
- (void)stopCapture;

@end