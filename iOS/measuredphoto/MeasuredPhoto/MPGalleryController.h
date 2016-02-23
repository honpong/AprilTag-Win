//
//  MPGalleryController.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/16/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "MPEditPhoto.h"
#import "MPUndoOverlay.h"
#import "MPShareSheet.h"
#import <RCCore/RCCore.h>

@interface MPGalleryController : UIViewController <UICollectionViewDataSource, UICollectionViewDelegate, MPUndoOverlayDelegate, UIActionSheetDelegate, MPShareSheetDelegate, RCLocalMoviePlayerDelegate>

@property (weak, nonatomic) IBOutlet UIButton *cameraButton;
@property (weak, nonatomic) IBOutlet UIButton *menuButton;
@property (weak, nonatomic) IBOutlet UICollectionView *collectionView;
@property (nonatomic, readonly) UIView* zoomSourceView;
@property (weak, nonatomic) IBOutlet UIView *navBar;
@property (nonatomic, readonly) MPEditPhoto* editPhotoController;

- (IBAction)handleMenuButton:(id)sender;
- (IBAction)handleCameraButton:(id)sender;
- (IBAction)handleImageButton:(id)sender;
- (void) hideZoomedThumbnail;
- (void) gotoCapturePhoto;

@end
