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

@interface MPGalleryController ()

@end

@implementation MPGalleryController
{
    NSArray* measuredPhotos;
}

- (id) initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void) viewDidLoad
{
    [super viewDidLoad];
    measuredPhotos = [MPDMeasuredPhoto MR_findAllSortedBy:@"created_at" ascending:NO];
}

/*
#pragma mark - Navigation

// In a storyboard-based application, you will often want to do a little preparation before navigation
- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    // Get the new view controller using [segue destinationViewController].
    // Pass the selected object to the new view controller.
}
*/

- (IBAction)handleCameraButton:(id)sender
{
    UIViewController* vc = [self.storyboard instantiateViewControllerWithIdentifier:@"Camera"];
    self.view.window.rootViewController = vc;
}

#pragma mark - UICollectionView Datasource

- (NSInteger)collectionView:(UICollectionView *)view numberOfItemsInSection:(NSInteger)section
{
    return measuredPhotos.count;
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    MPGalleryCell* cell = (MPGalleryCell*)[collectionView dequeueReusableCellWithReuseIdentifier:@"photoCell" forIndexPath:indexPath];
    
    MPDMeasuredPhoto* measuredPhoto = measuredPhotos[indexPath.row];
    
    [cell setImage: [UIImage imageWithContentsOfFile:measuredPhoto.imageFileName]];
//    [cell setTitle: measuredPhoto.name];
    
    return cell;
}

@end
