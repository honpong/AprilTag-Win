//
//  QRDelegate.h
//  RC3DKSampleAR
//
//  Created by Brian on 11/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface QRDelegate : NSObject <AVCaptureMetadataOutputObjectsDelegate>

- (void) captureOutput:(AVCaptureOutput *)captureOutput didOutputMetadataObjects:(NSArray *)metadataObjects fromConnection:(AVCaptureConnection *)connection;

@end
