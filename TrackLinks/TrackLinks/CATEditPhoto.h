//  CATEditPhoto.h
//  TrackLinks
//
//  Created by Ben Hirashima on 3/18/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import "RC3DK.h"
#import "CATHttpInterceptor.h"
#import "CATMeasuredPhoto.h"

@interface CATEditPhoto : UIViewController <UIWebViewDelegate, CATHttpInterceptorDelegate, UITextFieldDelegate>

@property (weak, nonatomic) IBOutlet UIWebView *webView;
@property (weak, nonatomic) IBOutlet UIButton *cameraButton;
@property (weak, nonatomic) IBOutlet UIView *navBar;
@property (nonatomic, readonly) UIDeviceOrientation currentUIOrientation;
@property (nonatomic) CATMeasuredPhoto* measuredPhoto;

- (IBAction)handleCameraButton:(id)sender;
- (void) setOrientation:(UIDeviceOrientation)orientation animated:(BOOL)animated;

@end
