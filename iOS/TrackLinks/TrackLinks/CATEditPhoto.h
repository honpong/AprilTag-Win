
//  Copyright (c) 2014 Caterpillar. All rights reserved.


#import "RC3DK.h"
#import "CATMeasuredPhoto.h"

@interface CATEditPhoto : UIViewController <UIWebViewDelegate, UITextFieldDelegate>

@property (weak, nonatomic) IBOutlet UIWebView *webView;
@property (weak, nonatomic) IBOutlet UIButton *cameraButton;
@property (nonatomic) CATMeasuredPhoto* measuredPhoto;

- (IBAction)handleCameraButton:(id)sender;

@end
