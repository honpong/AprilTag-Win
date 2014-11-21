//
//  QRDelegate.m
//  RC3DKSampleAR
//
//  Created by Brian on 11/7/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "QRDelegate.h"

@implementation  QRDelegate

- (void)captureOutput:(AVCaptureOutput *)captureOutput didOutputMetadataObjects:(NSArray *)metadataObjects fromConnection:(AVCaptureConnection *)connection
{
    for(AVMetadataMachineReadableCodeObject *metadata in metadataObjects)
    {
#warning Make sure you set this to the actual size of your QR code (width or height in meters)
        [[RCSensorFusion sharedInstance] requestTransformationForQRCodeObservation:metadata withDimension:.1825];
        break; //This could include up to 4 QR detections in the frame, but we just want the first one.
    }
}

@end
