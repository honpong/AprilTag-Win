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
        [[RCSensorFusion sharedInstance] getTransformationForQRCodeObservation:metadata transformOutput:true];
        break; //This could include up to 4 QR detections in the frame, but we just want the first one.
    }
}

@end
