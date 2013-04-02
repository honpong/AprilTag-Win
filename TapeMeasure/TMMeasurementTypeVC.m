//
//  TMMeasurementTypeVCViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/30/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMMeasurementTypeVC.h"

@interface TMMeasurementTypeVC ()

@end

@implementation TMMeasurementTypeVC

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    shouldEndAVSession = YES;
    
    [self.collectionView registerClass:[UICollectionViewCell class] forCellWithReuseIdentifier:@"BasicCell"];
}

- (void)viewWillAppear:(BOOL)animated
{
    [self.navigationController setToolbarHidden:YES animated:animated];
    [super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated
{
    [TMAnalytics logEvent:@"View.ChooseType"];
    [super viewDidAppear:animated];
    [SESSION_MANAGER startSession];
}

- (void)viewWillDisappear:(BOOL)animated
{
    if (shouldEndAVSession) [SESSION_MANAGER endSession];
}

- (void)viewDidUnload {
    [self setScrollView:nil];
    [super viewDidUnload];
}

//- (IBAction)handlePointToPoint:(id)sender
//{
//    type = TypePointToPoint;
//    [self performSegueWithIdentifier:@"toNew" sender:self];
//}
//
//- (IBAction)handleTotalPath:(id)sender
//{
//    type = TypeTotalPath;
//    [self performSegueWithIdentifier:@"toNew" sender:self];
//}
//
//- (IBAction)handleHorizontal:(id)sender
//{
//    type = TypeHorizontal;
//    [self performSegueWithIdentifier:@"toNew" sender:self];
//}
//
//- (IBAction)handleVertical:(id)sender
//{
//    type = TypeVertical;
//    [self performSegueWithIdentifier:@"toNew" sender:self];
//}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if([[segue identifier] isEqualToString:@"toNew"])
    {
        shouldEndAVSession = NO;
        
        newVC = [segue destinationViewController];
        newVC.type = type;
    }
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
{
    return ((toInterfaceOrientation == UIInterfaceOrientationPortrait) ||
            (toInterfaceOrientation == UIInterfaceOrientationPortraitUpsideDown) ||
            (toInterfaceOrientation == UIInterfaceOrientationLandscapeLeft) ||
            (toInterfaceOrientation == UIInterfaceOrientationLandscapeRight));
}

- (NSUInteger)supportedInterfaceOrientations
{
    return (UIInterfaceOrientationMaskAll);
}

#pragma mark - UICollectionView Datasource

- (NSInteger)collectionView:(UICollectionView *)view numberOfItemsInSection:(NSInteger)section
{
    return 4;
}

- (NSInteger)numberOfSectionsInCollectionView: (UICollectionView *)collectionView
{
    return 1;
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)cv cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    UICollectionViewCell *cell = [cv dequeueReusableCellWithReuseIdentifier:@"BasicCell " forIndexPath:indexPath];
    UILabel label = (UILabel)[cell viewWithTag:1];
    label.text = @"Point to Point";
    return cell;
}

#pragma mark - UICollectionViewDelegate

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
    // TODO: Select Item
}

- (void)collectionView:(UICollectionView *)collectionView didDeselectItemAtIndexPath:(NSIndexPath *)indexPath
{
    // TODO: Deselect item
}

#pragma mark – UICollectionViewDelegateFlowLayout

- (CGSize)collectionView:(UICollectionView *)collectionView layout:(UICollectionViewLayout*)collectionViewLayout sizeForItemAtIndexPath:(NSIndexPath *)indexPath
{
    return CGSizeMake(155, 155);
}

- (UIEdgeInsets)collectionView:(UICollectionView *)collectionView layout:(UICollectionViewLayout*)collectionViewLayout insetForSectionAtIndex:(NSInteger)section
{
    return UIEdgeInsetsMake(50, 20, 50, 20);
}

@end
