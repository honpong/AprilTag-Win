//
//  TMMeasurementTypeVCViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/30/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMMeasurementTypeVC.h"

@implementation TMMeasurementTypeVC
{
    MeasurementType type;
    TMNewMeasurementVC *newVC;
    bool shouldEndAVSession;
    NSArray* measurementTypes;
}

static NSString *CellIdentifier = @"MeasurementTypeCell";

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
    
    [self buildTypesArray];
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

- (void) buildTypesArray
{
    measurementTypes = @[
    [[TMMeasurementType alloc] initWithName:@"Point to Point" withImageName:@"PointToPoint" withType:TypePointToPoint],
    [[TMMeasurementType alloc] initWithName:@"Total Path" withImageName:@"TotalPath" withType:TypeTotalPath],
    [[TMMeasurementType alloc] initWithName:@"Horizontal" withImageName:@"Horz" withType:TypeHorizontal],
    [[TMMeasurementType alloc] initWithName:@"Vertical" withImageName:@"Vert" withType:TypeVertical]
    ];
}

#pragma mark - UICollectionView Datasource

- (NSInteger)collectionView:(UICollectionView *)view numberOfItemsInSection:(NSInteger)section
{
    return measurementTypes.count;
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    TMMeasurementTypeCell* cell = (TMMeasurementTypeCell*)[collectionView dequeueReusableCellWithReuseIdentifier:CellIdentifier forIndexPath:indexPath];
    
    TMMeasurementType* measurementType = measurementTypes[indexPath.row];
    
    [cell setImage: [UIImage imageNamed:measurementType.imageName]];
    [cell setText: measurementType.name];
    cell.type = measurementType.type;
    
    return cell;
}

#pragma mark - UICollectionViewDelegate

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
    NSLog(@"Selected %@", indexPath);
}

- (void)collectionView:(UICollectionView *)collectionView didDeselectItemAtIndexPath:(NSIndexPath *)indexPath
{
    NSLog(@"Deselected %@", indexPath);
}

//#pragma mark – UICollectionViewDelegateFlowLayout
//
//- (CGSize)collectionView:(UICollectionView *)collectionView layout:(UICollectionViewLayout*)collectionViewLayout sizeForItemAtIndexPath:(NSIndexPath *)indexPath
//{
//    return CGSizeMake(145, 145);
//}
//
//- (UIEdgeInsets)collectionView:(UICollectionView *)collectionView layout:(UICollectionViewLayout*)collectionViewLayout insetForSectionAtIndex:(NSInteger)section
//{
//    return UIEdgeInsetsMake(10, 10, 10, 10);
//}

@end
