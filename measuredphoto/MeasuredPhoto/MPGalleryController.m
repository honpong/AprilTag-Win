//
//  MPGalleryController.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/16/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#import "MPGalleryController.h"
#import "CoreData+MagicalRecord.h"
#import "MPDMeasuredPhoto+MPDMeasuredPhotoExt.h"
#import "MPGalleryCell.h"
#import "MPEditPhoto.h"
#import "MPCapturePhoto.h"
#import "MPZoomTransitionDelegate.h"

@interface MPGalleryController ()

@property (nonatomic, readwrite) UIView* transitionFromView;

@end

@implementation MPGalleryController
{
    NSArray* measuredPhotos;
    MPEditPhoto* editPhotoController;
    MPZoomTransitionDelegate* transitionDelegate;
}

- (void) viewDidLoad
{
    [super viewDidLoad];
    measuredPhotos = [MPDMeasuredPhoto MR_findAllSortedBy:@"created_at" ascending:NO];
    self.collectionView.dataSource = self;
    self.collectionView.delegate = self;
}

- (void) viewDidAppear:(BOOL)animated
{
    measuredPhotos = [MPDMeasuredPhoto MR_findAllSortedBy:@"created_at" ascending:NO];
    [self.collectionView reloadData];
}

#pragma mark - Event handlers

- (IBAction)handleMenuButton:(id)sender
{
    
}

- (IBAction)handleCameraButton:(id)sender
{
    MPCapturePhoto* vc = [self.storyboard instantiateViewControllerWithIdentifier:@"Camera"];
    UIDeviceOrientation deviceOrientation = [UIView deviceOrientationFromUIOrientation:self.interfaceOrientation];
    [vc setOrientation:deviceOrientation animated:NO];
    [self presentViewController:vc animated:YES completion:nil];
}

- (IBAction)handleImageButton:(id)sender
{
    UIButton* button = (UIButton*)sender;
    MPGalleryCell* cell = (MPGalleryCell*)button.superview.superview;
    MPDMeasuredPhoto* measuredPhoto = measuredPhotos[cell.index];
    
//    self.transitionFromView = cell;
//    transitionDelegate = [MPZoomTransitionDelegate new];
    
    if (editPhotoController == nil) editPhotoController = [self.storyboard instantiateViewControllerWithIdentifier:@"EditPhoto"];
    editPhotoController.measuredPhoto = measuredPhoto;
    editPhotoController.delegate = self;
    //    editPhotoController.transitioningDelegate = transitionDelegate;
    
    UIImageView* photo = [[UIImageView alloc] initWithFrame:cell.imgButton.imageView.frame];
    photo.center = cell.center;
    [photo setImage:cell.imgButton.imageView.image];
    [self.view insertSubview:photo  aboveSubview:self.collectionView];
    
    self.transitionFromView = photo;
    CGFloat scaleFactor;
    CGFloat x, y;
    x = self.collectionView.bounds.size.width;
    y = (1.33333333) * x;
    
    if (self.transitionFromView.bounds.size.width > self.transitionFromView.bounds.size.height)
    {
        scaleFactor = self.collectionView.bounds.size.width / self.transitionFromView.bounds.size.width;
    }
    else
    {
        scaleFactor = self.collectionView.bounds.size.height / self.transitionFromView.bounds.size.height;
    }
    
//    CGAffineTransform scale = CGAffineTransformMakeScale(scaleFactor,  scaleFactor);
//    CGAffineTransform translation = CGAffineTransformMakeTranslation(self.collectionView.center.x - self.transitionFromView.center.x, self.collectionView.center.y - self.transitionFromView.center.y);
//    CGAffineTransform transform = CGAffineTransformTranslate(scale, self.collectionView.center.x - self.transitionFromView.center.x, self.collectionView.center.y - self.transitionFromView.center.y);
    [UIView animateWithDuration: .3
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
//                         self.transitionFromView.transform = CGAffineTransformConcat(scale, translation);
//                         self.transitionFromView.transform = scale;
//                         self.transitionFromView.center = self.collectionView.center;
                         self.transitionFromView.bounds = CGRectMake(0, 0, x, y);
                         self.transitionFromView.center = self.collectionView.center;
                     }
                     completion:^(BOOL finished){
                         [self presentViewController:editPhotoController animated:NO completion:nil];
                         [self.transitionFromView removeFromSuperview];
                     }];
}

#pragma mark - MPEditPhotoDelegate

- (void) didFinishEditingPhoto
{
    [self dismissViewControllerAnimated:YES completion:nil];
}

#pragma mark - UICollectionView Datasource

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView
{
    return 1;
}

- (NSInteger)collectionView:(UICollectionView *)view numberOfItemsInSection:(NSInteger)section
{
    return measuredPhotos.count;
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    MPGalleryCell* cell = (MPGalleryCell*)[collectionView dequeueReusableCellWithReuseIdentifier:@"photoCell" forIndexPath:indexPath];
    
    MPDMeasuredPhoto* measuredPhoto = measuredPhotos[indexPath.row];
    
    cell.index = indexPath.row;
    cell.guid = measuredPhoto.id_guid;
    [cell setImage: [UIImage imageWithContentsOfFile:measuredPhoto.imageFileName]];
    [cell setTitle: measuredPhoto.name];
    
    return cell;
}

@end
