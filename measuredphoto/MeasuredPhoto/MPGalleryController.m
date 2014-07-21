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

@interface MPGalleryController ()

@end

@implementation MPGalleryController
{
    NSArray* measuredPhotos;
    MPEditPhoto* editPhotoController;
}

- (void) viewDidLoad
{
    [super viewDidLoad];
    measuredPhotos = [MPDMeasuredPhoto MR_findAllSortedBy:@"created_at" ascending:NO];
    self.collectionView.dataSource = self;
    self.collectionView.delegate = self;
}

#pragma mark - Event handlers

- (IBAction)handleMenuButton:(id)sender
{
    
}

- (IBAction)handleCameraButton:(id)sender
{
    UIViewController* vc = [self.storyboard instantiateViewControllerWithIdentifier:@"Camera"];
    self.view.window.rootViewController = vc;
}

- (IBAction)handleImageButton:(id)sender
{
    UIButton* button = (UIButton*)sender;
    MPGalleryCell* cell = (MPGalleryCell*)button.superview.superview;
    MPDMeasuredPhoto* measuredPhoto = measuredPhotos[cell.index];
    
    if (editPhotoController == nil) editPhotoController = [self.storyboard instantiateViewControllerWithIdentifier:@"EditPhoto"];
    editPhotoController.measuredPhoto = measuredPhoto;
    editPhotoController.delegate = self;
    [self presentViewController:editPhotoController animated:YES completion:nil];
}

#pragma mark - MPEditPhotoDelegate

- (void) didFinishEditingPhoto
{
    [self dismissViewControllerAnimated:editPhotoController completion:nil];
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
//    [cell setTitle: measuredPhoto.name];
    
    return cell;
}

@end
