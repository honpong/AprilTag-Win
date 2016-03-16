//
//  TMMeasurementTypeVCViewController.m
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/30/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMMeasurementTypeVC.h"
#import "TMAnalytics.h"

@implementation TMMeasurementTypeVC
{
    MeasurementType selectedType;
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
    LOGME
    [super viewDidLoad];
    [self buildTypesArray];
}

- (void)viewWillAppear:(BOOL)animated
{
    [self.navigationController setToolbarHidden:YES animated:animated];
    [super viewWillAppear:animated];
}

- (void)viewDidAppear:(BOOL)animated
{
    LOGME
    [super viewDidAppear:animated];
    
    [TMAnalytics logEvent:@"View.ChooseType"];
    self.navigationController.navigationBar.topItem.title = @"Choose Type";
    [SESSION_MANAGER startSession];
    shouldEndAVSession = YES;
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(handleResume)
                                                 name:UIApplicationDidBecomeActiveNotification
                                               object:nil];
}

- (void)viewWillDisappear:(BOOL)animated
{
    LOGME
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    if (shouldEndAVSession) {
        [SESSION_MANAGER endSession];
    }
}

- (void)handleResume
{
    LOGME
    [SESSION_MANAGER startSession]; //don't worry about ending the session. RCAVSessionManager automatically handles that on pause.
}

- (void)prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender
{
    if([[segue identifier] isEqualToString:@"toNew"])
    {
        shouldEndAVSession = NO;
        
        newVC = [segue destinationViewController];
        newVC.type = selectedType;
    }
    else
    {
        shouldEndAVSession = YES;
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

- (void) didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
    //prevents table view from being partially covered by navigation bar on orientation change in iOS 6
    [self.navigationController.view layoutSubviews];
}

- (void) buildTypesArray
{
    measurementTypes = @[
        [[TMMeasurementType alloc] initWithName:@"Point to Point" withImageName:@"PointToPoint" withType:TypePointToPoint],
        [[TMMeasurementType alloc] initWithName:@"Total Length" withImageName:@"TotalPath" withType:TypeTotalPath],
        [[TMMeasurementType alloc] initWithName:@"Horizontal" withImageName:@"Horizontal" withType:TypeHorizontal],
        [[TMMeasurementType alloc] initWithName:@"Vertical" withImageName:@"Vertical" withType:TypeVertical]
    ];
}

- (IBAction)handleButton:(id)sender
{
    UIButton* button = (UIButton*)sender;
    TMMeasurementTypeCell* cell = (TMMeasurementTypeCell*)button.superview.superview;
    selectedType = cell.type;
    [self performSegueWithIdentifier:@"toNew" sender:self];
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

@end
