//
//  TMMeasurementTypeVCViewController.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/30/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface TMMeasurementTypeVC : UIViewController
{
    MeasurementType type;
}
@property (weak, nonatomic) IBOutlet UIScrollView *scrollView;

- (IBAction)handlePointToPoint:(id)sender;
- (IBAction)handleTotalPath:(id)sender;
- (IBAction)handleHorizontal:(id)sender;
- (IBAction)handleVertical:(id)sender;
@end
