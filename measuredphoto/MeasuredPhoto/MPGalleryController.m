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
@property (nonatomic, readwrite) MPEditPhoto* editPhotoController;

@end

@implementation MPGalleryController
{
    NSMutableArray* measuredPhotos;
    MPFadeTransitionDelegate* fadeTransitionDelegate;
    UIView* shrinkToView;
    MPUndoOverlay* undoView;
    MPDMeasuredPhoto* photoToBeDeleted;
}

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void) viewDidLoad
{
    [super viewDidLoad];
    
    [self refreshCollectionData];
    self.collectionView.dataSource = self;
    self.collectionView.delegate = self;
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleNotification:)
                                                 name:MPCapturePhotoDidAppearNotification
                                               object:nil];
    
    fadeTransitionDelegate = [MPFadeTransitionDelegate new];
    _editPhotoController = [self.storyboard instantiateViewControllerWithIdentifier:@"EditPhoto"];
    [self.editPhotoController.view class]; // forces view to load, calling viewDidLoad:
    [self.editPhotoController setOrientation:[UIView deviceOrientationFromUIOrientation:self.interfaceOrientation] animated:NO];
    
    undoView = [[MPUndoOverlay alloc] initWithMessage:@"Photo deleted"];
    undoView.delegate = self;
    [self.view addSubview: undoView];
    
    photoToBeDeleted = nil;
}

- (void) viewDidAppear:(BOOL)animated
{
    if (self.editPhotoController.measuredPhoto.is_deleted)
    {
        [self handlePhotoDeleted];
    }
    else if (self.editPhotoController.measuredPhoto && ![measuredPhotos containsObject:self.editPhotoController.measuredPhoto])
    {
        [self handlePhotoAdded];
    }
    else
    {
        if (self.transitionFromView.superview)
        {
            [self zoomThumbnailOut];
        }
        [self refreshCollection];
    }
}

- (void) viewDidDisappear:(BOOL)animated
{
    self.collectionView.alpha = 1.;
    self.navBar.alpha = 1.;
    [undoView hide];
    [self handleUndoPeriodExpired]; // make sure any pending deletes happen right away
}

- (NSUInteger) supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskAll;
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
    [self presentViewController:vc animated:NO completion:nil];
}

- (IBAction)handleImageButton:(id)sender
{
    UIButton* button = (UIButton*)sender;
    MPGalleryCell* cell = (MPGalleryCell*)button.superview.superview;
    NSIndexPath* indexPath = [self.collectionView indexPathForCell:cell];
    MPDMeasuredPhoto* measuredPhoto = measuredPhotos[indexPath.item];
    
    self.editPhotoController.measuredPhoto = measuredPhoto;
    self.editPhotoController.indexPath = indexPath; // must be set after .measuredPhoto
    self.editPhotoController.transitioningDelegate = fadeTransitionDelegate;
    
    shrinkToView = cell;
    CGRect shrinkToFrame = [self.view convertRect:shrinkToView.frame fromView:self.collectionView];
    
    UIImageView* photo = [[UIImageView alloc] initWithFrame:shrinkToFrame];
    photo.center = cell.center;
    [photo setContentMode:UIViewContentModeScaleAspectFit];
    [photo setImage:cell.imgButton.imageView.image];
    
    [self.view insertSubview:photo atIndex:self.view.subviews.count];
    photo.frame = shrinkToFrame;
    
    self.transitionFromView = photo;
    
    [self zoomThumbnailIn:photo];
}

- (void) handleNotification:(NSNotification*)notification
{
    if ([notification.name isEqual:MPCapturePhotoDidAppearNotification])
    {
        [self.transitionFromView removeFromSuperview];
    }
}

- (void) handlePhotoDeleted
{
    photoToBeDeleted = self.editPhotoController.measuredPhoto;
    
    [self.transitionFromView removeFromSuperview];
    
    NSIndexPath* indexPath = self.editPhotoController.indexPath;
    
    if (indexPath) // if indexPath is nil, then the deleted photo was not opened from the gallery view
    {
        NSInteger itemIndex = indexPath.item;
        if (itemIndex < measuredPhotos.count) [measuredPhotos removeObjectAtIndex:itemIndex];
        [self.collectionView deleteItemsAtIndexPaths:[NSArray arrayWithObject:indexPath]];
    }
    [self.collectionView reloadData];
    
    [undoView showWithDuration:6.];
}

- (void) handlePhotoAdded
{
    int photosAdded = [self refreshCollectionData];
    NSMutableArray* indexPaths = [[NSMutableArray alloc] initWithCapacity:photosAdded];
    for (int i = 0; i < photosAdded; i++)
    {
        NSIndexPath* indexPath = [NSIndexPath indexPathForItem:i inSection:0];
        [indexPaths addObject:indexPath];
    }
   [self.collectionView insertItemsAtIndexPaths:indexPaths];
}

#pragma mark - MPUndoOverlayDelegate

- (void) handleUndoButton
{
    NSIndexPath* indexPath;
    
    photoToBeDeleted.is_deleted = NO;
    [self refreshCollectionData];
    
    if (self.editPhotoController.indexPath)
    {
        indexPath = self.editPhotoController.indexPath;
    }
    else
    {
        indexPath = [NSIndexPath indexPathForItem:0 inSection:0];
    }
    
    [self.collectionView insertItemsAtIndexPaths:[NSArray arrayWithObject:indexPath]];
    photoToBeDeleted = nil;
}

- (void) handleUndoPeriodExpired
{
    if (photoToBeDeleted)
    {
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
            [CONTEXT MR_saveToPersistentStoreWithCompletion:^(BOOL success, NSError *error) {
                if (success)
                {
                    DLog(@"Deleted %@", photoToBeDeleted.id_guid);
                    
                    if (![photoToBeDeleted deleteAssociatedFiles])
                    {
                        DLogs(@"Failed to delete files");
                        //TODO: log error to analytics
                    }
                }
                else if (error)
                {
                    DLog(@"Error saving context: %@", error);
                }
                
                photoToBeDeleted = nil;
                self.editPhotoController.measuredPhoto = nil;
            }];
        });
    }
}

#pragma mark - Animations

- (void) zoomThumbnailIn:(UIImageView*)photo
{
    CGFloat width, height;
    CGPoint center;
    
    if (UIInterfaceOrientationIsPortrait(self.interfaceOrientation))
    {
        width = self.collectionView.bounds.size.width;
        height = (1.33333333) * width;
        center = self.view.center;
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
                         [photo addMatchSuperviewConstraints];
                         [photo.superview setNeedsUpdateConstraints];
                         [photo.superview layoutIfNeeded];
                         
                         [self presentViewController:self.editPhotoController animated:YES completion:nil];
                     }];
}

- (void) zoomThumbnailOut
{
    if (self.transitionFromView.superview)
    {
        [self.transitionFromView removeConstraintsFromSuperview];
        [self.transitionFromView removeConstraints:self.transitionFromView.constraints];
        
        [UIView animateWithDuration: zoomAnimationDuration
                              delay: 0
                            options: UIViewAnimationOptionCurveEaseIn
                         animations:^{
                             self.transitionFromView.frame = [self.view convertRect:shrinkToView.frame fromView:self.collectionView];
                         }
                         completion:^(BOOL finished){
                             [self.transitionFromView removeFromSuperview];
                         }];
    }
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
    
    MPDMeasuredPhoto* measuredPhoto = measuredPhotos[indexPath.item];
    
    cell.guid = measuredPhoto.id_guid;
    [cell setImage: [UIImage imageWithContentsOfFile:measuredPhoto.imageFileName]];
    [cell setTitle: measuredPhoto.name];
    
    return cell;
}

- (int) refreshCollectionData
{
    int previousCount = measuredPhotos.count;
    NSPredicate* predicate = [NSPredicate predicateWithFormat:@"is_deleted == NO"];
    measuredPhotos = [[MPDMeasuredPhoto MR_findAllSortedBy:@"created_at" ascending:NO withPredicate:predicate] mutableCopy];
    return measuredPhotos.count - previousCount;
}

- (void) refreshCollection
{
    [self refreshCollectionData];
    [self.collectionView reloadData];
}

@end
