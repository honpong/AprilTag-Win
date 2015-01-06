//
//  Created by Ben Hirashima on 12/29/14.
//  Copyright (c) 2014 Sunlayar. All rights reserved.
//

#import "RC3DK.h"
#import "SLMeasuredPhoto.h"

@interface SLRoofController : UIViewController <UIWebViewDelegate, UITextFieldDelegate>

@property (weak, nonatomic) IBOutlet UIWebView *webView;
@property (weak, nonatomic) IBOutlet UIButton *cameraButton;
@property (nonatomic) SLMeasuredPhoto* measuredPhoto;

- (IBAction)handleBackButton:(id)sender;

@end
