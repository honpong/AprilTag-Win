//
//  SLAugRealityController.m
//  Sunlayar
//
//  Created by Ben Hirashima on 2/27/15.
//  Copyright (c) 2015 Sunlayar. All rights reserved.
//

#import "SLAugRealityController.h"
#import "RCDebugLog.h"

@interface SLAugRealityController ()

@end

@implementation SLAugRealityController

- (instancetype)init
{
    self = [super init];
    if (!self) return nil;
    
    pageURL = [[NSBundle mainBundle] URLForResource:@"aug_reality" withExtension:@"html"];
    
    return self;
}

- (BOOL)prefersStatusBarHidden { return YES; }

- (NSUInteger)supportedInterfaceOrientations { return UIInterfaceOrientationMaskLandscapeRight; }

#pragma mark - RCHttpInterceptorDelegate

- (NSDictionary *)handleAction:(ARNativeAction *)nativeAction error:(NSError **)error
{
    // allow superclass to handle this action. if it doesn't, then handle it ourselves.
    NSDictionary* result = [super handleAction:nativeAction error:error];
    if (result) return result;
    
    if ([nativeAction.request.URL.relativePath isEqualToString:@"/getRoofJsonFilePath"])
    {
        return @{ @"filePath": self.measuredPhoto.annotationsFileName };
    }
    
    return nil;
}

@end
