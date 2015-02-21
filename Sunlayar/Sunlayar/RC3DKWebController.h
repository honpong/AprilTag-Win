//
//  RC3DKWebController.h
//  Sunlayar
//
//  Created by Ben Hirashima on 2/20/15.
//  Copyright (c) 2015 Sunlayar. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MBProgressHUD.h"
#import "SLConstants.h"
#import "RCVideoPreview.h"
#import "RC3DK.h"
#import "RCHttpInterceptor.h"

@interface RC3DKWebController : UIViewController <RCSensorFusionDelegate, UIWebViewDelegate, RCHttpInterceptorDelegate>
{
    NSURL* pageURL;
}

@property (nonatomic, readonly) UIWebView* webView;
@property (nonatomic, readonly) RCVideoPreview* videoView;

@end
