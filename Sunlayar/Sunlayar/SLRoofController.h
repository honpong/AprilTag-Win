//
//  Created by Ben Hirashima on 12/29/14.
//  Copyright (c) 2014 Sunlayar. All rights reserved.
//

#import "RC3DK.h"
#import "SLMeasuredPhoto.h"
#import "RCHttpInterceptor.h"

@protocol SLRoofControllerDelegate <NSObject>

- (void) roofDefinitionComplete;

@end

@interface SLRoofController : UIViewController <UIWebViewDelegate, UITextFieldDelegate, RCHttpInterceptorDelegate>

@property (nonatomic) UIWebView *webView;
@property (nonatomic) SLMeasuredPhoto* measuredPhoto;

@end
