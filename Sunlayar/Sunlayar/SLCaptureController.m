//
//  SLARWebViewViewController.m
//  Sunlayar
//
//  Created by Ben Hirashima on 2/20/15.
//  Copyright (c) 2015 Sunlayar. All rights reserved.
//

#import "SLCaptureController.h"
#import "SLRoofController.h"

@implementation SLCaptureController

- (instancetype)init
{
    self = [super init];
    if (!self) return nil;
    
    pageURL = [[NSBundle mainBundle] URLForResource:@"capture" withExtension:@"html"];
    
    return self;
}

- (BOOL)prefersStatusBarHidden { return YES; }

- (NSUInteger)supportedInterfaceOrientations { return UIInterfaceOrientationMaskLandscapeRight; }

- (void) stereoDidFinish
{
    [super stereoDidFinish];
    [self performSelectorOnMainThread:@selector(gotoRoofController) withObject:nil waitUntilDone:NO];
}

- (void) gotoRoofController
{
    // cleanup
    self.webView.delegate = nil;
    [self.webView stopLoading];
    [self hideVideoView];
    
    SLRoofController* vc = [SLRoofController new];
    vc.measuredPhoto = self.measuredPhoto;
    self.view.window.rootViewController = vc;
}

@end
