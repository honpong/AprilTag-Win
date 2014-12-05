//  CATEditPhoto.h
//  TrackLinks
//
//  Created by Ben Hirashima on 3/18/14.
//  Copyright (c) 2014 Caterpillar. All rights reserved.
//

#import <RC3DK/RC3DK.h>
#import "CATHttpInterceptor.h"
#import "CATRotatingButton.h"

@class MPDMeasuredPhoto;

@interface CATEditPhoto : UIViewController <UIWebViewDelegate, CATHttpInterceptorDelegate, UITextFieldDelegate>

@property (weak, nonatomic) IBOutlet UIWebView *webView;
@property (weak, nonatomic) IBOutlet UIButton *photosButton;
@property (weak, nonatomic) IBOutlet UIButton *cameraButton;
@property (weak, nonatomic) IBOutlet UIButton *shareButton;
@property (weak, nonatomic) IBOutlet UIButton *deleteButton;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint *titleTextWidthConstraint;
@property (weak, nonatomic) IBOutlet UIView *navBar;
@property (nonatomic, readonly) UIDeviceOrientation currentUIOrientation;
@property (nonatomic) NSIndexPath* indexPath;

- (IBAction)handlePhotosButton:(id)sender;
- (IBAction)handleCameraButton:(id)sender;
- (IBAction)handleShareButton:(id)sender;
- (IBAction)handleDelete:(id)sender;
- (IBAction)handleTitleButton:(id)sender;
- (void) setOrientation:(UIDeviceOrientation)orientation animated:(BOOL)animated;

@end
