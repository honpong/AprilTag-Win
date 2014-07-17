//
//  MPGalleryController.h
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/16/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface MPGalleryController : UICollectionViewController

@property (weak, nonatomic) IBOutlet UIBarButtonItem *btnCamera;

- (IBAction)handleCameraButton:(id)sender;

@end
