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
#import "CustomIOS7AlertView.h"
#import "MPAboutView.h"
#import "MPLocalMoviePlayer.h"
#import "MPPreferencesController.h"
#import "MPWebViewController.h"
#import <QuartzCore/QuartzCore.h>

static const NSTimeInterval zoomAnimationDuration = .1;

@interface MPGalleryController ()

@property (nonatomic, readwrite) UIView* zoomedThumbnail;
@property (nonatomic, readwrite) MPEditPhoto* editPhotoController;

@end

@implementation MPGalleryController
{
    NSMutableArray* measuredPhotos;
    MPFadeTransitionDelegate* fadeTransitionDelegate;
    MPUndoOverlay* undoView;
    MPDMeasuredPhoto* photoToBeDeleted;
    MPShareSheet* shareSheet;
    UIActionSheet *actionSheet;
    RCModalCoverVerticalTransition* transition;
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
    self.editPhotoController.transitioningDelegate = fadeTransitionDelegate;
    [self.editPhotoController.view class]; // forces view to load, calling viewDidLoad:
    [self.editPhotoController setOrientation:[UIView deviceOrientationFromUIOrientation:self.interfaceOrientation] animated:NO];
    
    undoView = [[MPUndoOverlay alloc] initWithMessage:@"Photo deleted"];
    undoView.delegate = self;
    [self.view addSubview: undoView];
    
    photoToBeDeleted = nil;
}

- (void) viewDidAppear:(BOOL)animated
{
    [MPAnalytics logEvent:@"View.Gallery"];
    
    if (self.editPhotoController.measuredPhoto.is_deleted)
    {
        [self handlePhotoDeleted];
    }
    else if (self.editPhotoController.measuredPhoto && ![measuredPhotos containsObject:self.editPhotoController.measuredPhoto])
    {
        [self hideZoomedThumbnail];
        [self handlePhotoAdded];
    }
    else
    {
        [self refreshCollection];
    }
}

- (void) viewWillDisappear:(BOOL)animated
{
    if (photoToBeDeleted)
    {
        [self deletePhoto]; // make sure any pending deletes happen right away
    }
    
    self.collectionView.alpha = 1.;
    self.navBar.alpha = 1.;
    [undoView hide];
}

- (NSUInteger) supportedInterfaceOrientations
{
    return UIInterfaceOrientationMaskAll;
}

#pragma mark - Event handlers

- (IBAction)handleMenuButton:(id)sender
{
    if (actionSheet.isVisible)
        [self dismissActionSheet];
    else
        [self showActionSheet];
}

- (IBAction)handleCameraButton:(id)sender
{
    [self gotoCapturePhoto];
}

- (IBAction)handleImageButton:(id)sender
{
    UIButton* button = (UIButton*)sender;
    MPGalleryCell* cell = (MPGalleryCell*)button.superview.superview;
    NSIndexPath* indexPath = [self.collectionView indexPathForCell:cell];
    MPDMeasuredPhoto* measuredPhoto = measuredPhotos[indexPath.item];
    
    self.editPhotoController.measuredPhoto = measuredPhoto;
    self.editPhotoController.indexPath = indexPath; // must be set after .measuredPhoto
//    self.editPhotoController.modalPresentationStyle = UIModalPresentationCustom;
    
    _zoomSourceView = cell;
    CGRect shrinkToFrame = [self.view convertRect:self.zoomSourceView.frame fromView:self.collectionView];
    
    UIImageView* photo = [[UIImageView alloc] initWithFrame:shrinkToFrame];
    photo.center = cell.center;
    [photo setContentMode:UIViewContentModeScaleAspectFit];
    [photo setImage:cell.imgButton.imageView.image];
    
    [self.view insertSubview:photo atIndex:self.view.subviews.count];
    photo.frame = shrinkToFrame;
    
    self.zoomedThumbnail = photo;
    
    [self zoomThumbnailIn:photo];
}

- (void) handleNotification:(NSNotification*)notification
{
    if ([notification.name isEqual:MPCapturePhotoDidAppearNotification])
    {
        [self.zoomedThumbnail removeFromSuperview];
    }
}

- (void) handlePhotoDeleted
{
    NSIndexPath* indexPath = self.editPhotoController.indexPath;
    photoToBeDeleted = self.editPhotoController.measuredPhoto;
    self.editPhotoController.measuredPhoto = nil;
    
    [self.zoomedThumbnail removeFromSuperview];    
    
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

- (void) deletePhoto
{
    LOGME
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_BACKGROUND, 0), ^{
        [CONTEXT MR_saveToPersistentStoreWithCompletion:^(BOOL success, NSError *error) {
            if (success)
            {
                DLog(@"Deleted %@", photoToBeDeleted.id_guid);
                
                if (![photoToBeDeleted deleteAssociatedFiles])
                {
                    DLog(@"Failed to delete files");
                    //TODO: log error to analytics
                }
            }
            else if (error)
            {
                DLog(@"Error saving context: %@", error);
            }
            
            photoToBeDeleted = nil;
        }];
    });
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
    if (photoToBeDeleted) [self deletePhoto];
}

#pragma mark - Animations

- (void) zoomThumbnailIn:(UIImageView*)photo
{
    CGFloat width, height;
    
    BOOL isPhotoPortrait = photo.image.size.width < photo.image.size.height;
    
    if (UIInterfaceOrientationIsPortrait(self.interfaceOrientation))
    {
        if (isPhotoPortrait)
        {
            height = self.view.bounds.size.height - 88;
            width = self.view.bounds.size.width;
        }
        else
        {
            width = self.view.bounds.size.width;
            height = self.view.bounds.size.height;
        }
    }
    else // landscape
    {
        if (isPhotoPortrait)
        {
            height = self.view.bounds.size.height;
            width = self.view.bounds.size.width;
        }
        else
        {
            width = self.view.bounds.size.width - 88;
            height = self.view.bounds.size.height;
        }
    }
    
    [UIView animateWithDuration: zoomAnimationDuration
                          delay: 0
                        options: UIViewAnimationOptionCurveEaseIn
                     animations:^{
                         [photo addCenterInSuperviewConstraints];
                         [photo addWidthConstraint:width andHeightConstraint:height];
                         [photo.superview setNeedsUpdateConstraints];
                         [photo.superview layoutIfNeeded];
                         self.collectionView.alpha = 0;
                         if (UIInterfaceOrientationIsLandscape(self.interfaceOrientation)) self.navBar.alpha = 0;
                     }
                     completion:^(BOOL finished){
                         self.collectionView.hidden = YES;
                         [self presentViewController:self.editPhotoController animated:YES completion:^{
                             [self hideZoomedThumbnail];
                             self.collectionView.hidden = NO;
                             [MPAnalytics logEvent:@"View.EditPhoto"];
                         }];
                     }];
}

//- (void) zoomThumbnailOut
//{
//    if (self.zoomedThumbnail.superview)
//    {
//        [self.zoomedThumbnail removeConstraintsFromSuperview];
//        [self.zoomedThumbnail removeConstraints:self.zoomedThumbnail.constraints];
//        
//        [UIView animateWithDuration: zoomAnimationDuration
//                              delay: 0
//                            options: UIViewAnimationOptionCurveEaseOut
//                         animations:^{
//                             CGRect frame = [self.view convertRect:self.zoomSourceView.frame fromView:self.collectionView];
//                             [self.zoomedThumbnail addLeftSpaceToSuperviewConstraint:frame.origin.x];
//                             [self.zoomedThumbnail addTopSpaceToSuperviewConstraint:frame.origin.y];
//                             [self.zoomedThumbnail addWidthConstraint:frame.size.width andHeightConstraint:frame.size.height];
//                             [self.zoomedThumbnail.superview setNeedsUpdateConstraints];
//                             [self.zoomedThumbnail setNeedsUpdateConstraints];
//                             [self.zoomedThumbnail layoutIfNeeded];
//                             [self.zoomedThumbnail.superview layoutIfNeeded];
//                         }
//                         completion:^(BOOL finished){
//                             [self hideZoomedThumbnail];
//                         }];
//    }
//}

- (void) hideZoomedThumbnail
{
    if (self.zoomedThumbnail.superview)
    {
        [self.zoomedThumbnail removeFromSuperview];
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

#pragma mark - Action sheet

- (void)showActionSheet
{
    [MPAnalytics logEvent:@"View.History.Menu"];
    
    if (actionSheet == nil)
    {
        actionSheet = [[UIActionSheet alloc] initWithTitle:@"3Dimension Menu"
                                                  delegate:self
                                         cancelButtonTitle:@"Cancel"
                                    destructiveButtonTitle:nil
                                         otherButtonTitles:@"About", @"Share app", @"Rate the app", @"Accuracy tips", @"Tutorial video", @"Preferences", nil];
    }
    
    // Show the sheet
    [actionSheet showFromRect:self.menuButton.bounds inView:self.menuButton animated:YES];
}

- (void) dismissActionSheet
{
    [actionSheet dismissWithClickedButtonIndex:-1 animated:NO];
}

- (void)actionSheet:(UIActionSheet *)actionSheet didDismissWithButtonIndex:(NSInteger)buttonIndex
{
    switch (buttonIndex)
    {
        case 0:
        {
            [MPAnalytics logEvent:@"View.About"];
            [self gotoAbout];
            break;
        }
        case 1:
        {
            [MPAnalytics logEvent:@"View.ShareApp"];
            [self showShareSheet];
            break;
        }
        case 2:
        {
            [MPAnalytics logEvent:@"View.Rate"];
            [self gotoAppStore];
            break;
        }
        case 3:
        {
            [MPAnalytics logEvent:@"View.Tips"];
            [self gotoTips];
            break;
        }
        case 4:
        {
            [self gotoTutorialVideo];
            break;
        }
        case 5:
        {
            [self gotoPreferences];
            break;
        }
            
        default:
            break;
    }
}

- (void) gotoAbout
{
    MPWebViewController* viewController = [self.storyboard instantiateViewControllerWithIdentifier:@"GenericWebView"];
    viewController.htmlUrl = [[NSBundle mainBundle] URLForResource:@"about" withExtension:@"html"];
    [self presentViewController:viewController animated:YES completion:nil];
    viewController.titleLabel.text = @"About 3Dimension"; // must come after present...
}

- (void) gotoTips
{
    MPWebViewController* viewController = [self.storyboard instantiateViewControllerWithIdentifier:@"GenericWebView"];
    viewController.htmlUrl = [[NSBundle mainBundle] URLForResource:@"tips" withExtension:@"html"];
    [self presentViewController:viewController animated:YES completion:nil];
    viewController.titleLabel.text = @"Tips"; // must come after present...
}

- (void) gotoPreferences
{
    if (UI_USER_INTERFACE_IDIOM() == UIUserInterfaceIdiomPad)
    {
        [self performSegueWithIdentifier:@"toSettings" sender:self];
    }
    else
    {
        MPPreferencesController* prefsController = [self.storyboard instantiateViewControllerWithIdentifier:@"Preferences"];
        if (transition == nil) transition = [RCModalCoverVerticalTransition new];
        prefsController.transitioningDelegate = transition;
        prefsController.modalPresentationStyle = UIModalPresentationCustom;
        [self presentViewController:prefsController animated:YES completion:nil];
    }
}

- (void) gotoAppStore
{
    NSURL *url = [NSURL URLWithString:URL_APPSTORE];
    [[UIApplication sharedApplication] openURL:url];
}

- (void) gotoTutorialVideo
{
    MPLocalMoviePlayer* movieViewController = [self.storyboard instantiateViewControllerWithIdentifier:@"Tutorial"];
    movieViewController.delegate = self;
    [self presentViewController:movieViewController animated:YES completion:nil];
}

- (void)gotoCapturePhoto
{
    MPCapturePhoto* vc = [self.storyboard instantiateViewControllerWithIdentifier:@"Camera"];
    UIDeviceOrientation deviceOrientation = [UIView deviceOrientationFromUIOrientation:self.interfaceOrientation];
    [vc setOrientation:deviceOrientation animated:NO];
    [self presentViewController:vc animated:NO completion:nil];
    [MPAnalytics logEvent:@"View.CapturePhoto"];
}

#pragma mark - Sharing

- (NSString*) composeSharingString
{
    NSString* result = [NSString stringWithFormat: @"Check out this app that lets you measure anything in a photo: 3Dimension %@", URL_SHARING];
    return result;
}

- (void) showShareSheet
{
    OSKShareableContent *content = [OSKShareableContent contentFromText:[self composeSharingString]];
    content.title = @"Share App";
    shareSheet = [MPShareSheet shareSheetWithDelegate:self];
    [OSKActivitiesManager sharedInstance].customizationsDelegate = shareSheet;
    [shareSheet showShareSheet_Pad_FromRect:self.menuButton.frame withViewController:self inView:self.view content:content];
}

#pragma mark - TMShareSheetDelegate

- (OSKActivityCompletionHandler) activityCompletionHandler
{
    OSKActivityCompletionHandler activityCompletionHandler = ^(OSKActivity *activity, BOOL successful, NSError *error){
        if (successful) {
            [MPAnalytics logEvent:@"Share.App" withParameters:@{ @"Type": [activity.class activityName] }];
        } else {
            [MPAnalytics logError:@"Share.App" message:[activity.class activityName] error:error];
        }
    };
    return activityCompletionHandler;
}

#pragma mark - RCLocalMoviePlayerDelegate

- (void) moviePlayerContinueButtonTapped
{
    [self gotoCapturePhoto];
}

- (void) moviePlayerDismissed
{
    [self dismissViewControllerAnimated:YES completion:nil];
}

@end
