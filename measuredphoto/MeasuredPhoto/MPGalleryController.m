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
#import "MPFadeTransitionDelegate.h"

static const NSTimeInterval zoomAnimationDuration = .1;

@interface MPGalleryController ()

@property (nonatomic, readwrite) UIView* transitionFromView;

@end

@implementation MPGalleryController
{
    NSArray* measuredPhotos;
    MPEditPhoto* editPhotoController;
    MPFadeTransitionDelegate* transitionDelegate;
    CGRect shrinkToFrame;
}

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) viewDidLoad
{
    [super viewDidLoad];
    measuredPhotos = [MPDMeasuredPhoto MR_findAllSortedBy:@"created_at" ascending:NO];
    self.collectionView.dataSource = self;
    self.collectionView.delegate = self;
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleNotification:)
                                                 name:MPCapturePhotoDidAppearNotification
                                               object:nil];
    
    transitionDelegate = [MPFadeTransitionDelegate new];
    editPhotoController = [self.storyboard instantiateViewControllerWithIdentifier:@"EditPhoto"];
    editPhotoController.delegate = self;
    editPhotoController.transitioningDelegate = transitionDelegate;
    [editPhotoController.view class]; // forces view to load, calling viewDidLoad:
}

- (void) viewDidAppear:(BOOL)animated
{
    if (self.transitionFromView.superview)
    {
        [UIView animateWithDuration: zoomAnimationDuration
                              delay: 0
                            options: UIViewAnimationOptionCurveEaseIn
                         animations:^{
                             self.transitionFromView.frame = shrinkToFrame;
                         }
                         completion:^(BOOL finished){
                             [self.transitionFromView removeFromSuperview];
                         }];
    }
    
    measuredPhotos = [MPDMeasuredPhoto MR_findAllSortedBy:@"created_at" ascending:NO];
    [self.collectionView reloadData];
}

- (void) viewDidDisappear:(BOOL)animated
{
    self.collectionView.alpha = 1.;
    self.navBar.alpha = 1.;
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
    
    editPhotoController.measuredPhoto = measuredPhoto;
    
    UIImageView* photo = [[UIImageView alloc] initWithFrame:cell.frame];
    photo.center = cell.center;
    [photo setContentMode:UIViewContentModeScaleAspectFit];
    [photo setImage:cell.imgButton.imageView.image];
    
    shrinkToFrame = [self.view convertRect:photo.frame fromView:self.collectionView];
    [self.view insertSubview:photo atIndex:self.view.subviews.count];
    photo.frame = shrinkToFrame;
    
    self.transitionFromView = photo;
    
    CGFloat width, height;
    CGPoint center;
    
    if (UIInterfaceOrientationIsPortrait(self.interfaceOrientation))
    {
        width = self.collectionView.bounds.size.width;
        height = (1.33333333) * width;
        center = self.collectionView.center;
    }
    else
    {
        height = self.view.bounds.size.height;
        width = height * (1.33333333);
        center = CGPointMake(self.view.center.y, self.view.center.x); // not sure why x and y need to be swapped here. self.view isn't rotated?
    }
    
    [UIView animateWithDuration: zoomAnimationDuration
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         self.transitionFromView.bounds = CGRectMake(0, 0, width, height);
                         self.transitionFromView.center = center;
                         self.collectionView.alpha = 0;
                         if (UIInterfaceOrientationIsLandscape(self.interfaceOrientation)) self.navBar.alpha = 0;
                     }
                     completion:^(BOOL finished){
                         [self presentViewController:editPhotoController animated:YES completion:nil];
                     }];
}

- (void) handleNotification:(NSNotification*)notification
{
    if ([notification.name isEqual:MPCapturePhotoDidAppearNotification])
    {
        [self.transitionFromView removeFromSuperview];
    }
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
