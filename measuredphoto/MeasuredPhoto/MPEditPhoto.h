//
//  MPEditPhoto.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 3/18/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPViewController.h"
#import <RC3DK/RC3DK.h>
#import "MPHttpInterceptor.h"

@protocol MPEditPhotoDelegate <NSObject>

- (void) didFinishEditingPhoto;

@end

@class MPDMeasuredPhoto;

@interface MPEditPhoto : MPViewController <UIWebViewDelegate, MPHttpInterceptorDelegate>

@property (nonatomic) id<MPEditPhotoDelegate> delegate;
@property (nonatomic) MPDMeasuredPhoto* measuredPhoto;
@property (weak, nonatomic) IBOutlet UIWebView *webView;
@property (weak, nonatomic) IBOutlet UIButton *photosButton;
@property (weak, nonatomic) IBOutlet NSLayoutConstraint *cameraButton;
@property (weak, nonatomic) IBOutlet UIButton *shareButton;
@property (weak, nonatomic) IBOutlet UIButton *deleteButton;

- (IBAction)handlePhotosButton:(id)sender;
- (IBAction)handleCameraButton:(id)sender;
- (IBAction)handleShareButton:(id)sender;
- (IBAction)handleDelete:(id)sender;

@end
