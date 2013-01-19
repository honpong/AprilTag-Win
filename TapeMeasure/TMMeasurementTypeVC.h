//
//  TMMeasurementTypeVCViewController.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/30/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMNewMeasurementVC.h"
#import "TMAppDelegate.h"

@class TMNewMeasurementVC;

@interface TMMeasurementTypeVC : UIViewController
{
    MeasurementType type;
    TMNewMeasurementVC *newVC;
}
@property (weak, nonatomic) IBOutlet UIScrollView *scrollView;

- (IBAction)handlePointToPoint:(id)sender;
- (IBAction)handleTotalPath:(id)sender;
- (IBAction)handleHorizontal:(id)sender;
- (IBAction)handleVertical:(id)sender;
@end
