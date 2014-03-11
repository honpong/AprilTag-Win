//
//  CaptureController.h
//  RCCapture
//
//  Created by Brian on 10/3/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

#import "CaptureFile.h"

@protocol RCCaptureManagerDelegate <NSObject>

- (void) captureDidStop;
@optional
- (void) captureDidStart;

@end

@interface RCCaptureManager : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>

@property (weak, nonatomic) id<RCCaptureManagerDelegate> delegate;

- (void)startCapture:(NSString *)path withSession:(AVCaptureSession *)session withDevice:(AVCaptureDevice *)device withDelegate:(id<RCCaptureManagerDelegate>)delegate;
- (void)stopCapture;

@end