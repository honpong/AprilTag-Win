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

@interface TMMeasurementTypeVC : UICollectionViewController <UICollectionViewDataSource, UICollectionViewDelegateFlowLayout>
{
    MeasurementType type;
    TMNewMeasurementVC *newVC;
    bool shouldEndAVSession;
}

@end
