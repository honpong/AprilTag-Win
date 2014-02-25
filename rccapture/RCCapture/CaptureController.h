//
//  CaptureController.h
//  RCCapture
//
//  Created by Brian on 10/3/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <AVFoundation/AVFoundation.h>

#include "CaptureFile.h"

@protocol CaptureControllerDelegate <NSObject>

- (void) captureDidStop;
@optional
- (void) captureDidStart;

@end

@interface CaptureController : NSObject <AVCaptureVideoDataOutputSampleBufferDelegate>

@property (weak, nonatomic) id<CaptureControllerDelegate> delegate;

- (void)startCapture:(NSString *)path withSession:(AVCaptureSession *)session withDevice:(AVCaptureDevice *)device withDelegate:(id<CaptureControllerDelegate>)delegate;
- (void)stopCapture;

@end