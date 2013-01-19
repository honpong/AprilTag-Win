//
//  TMMeasurementsList.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMMeasurement.h"
#import "TMAppDelegate.h"
#import "TMResultsVC.h"
#import "TMDistanceFormatter.h"
#import "TMAppDelegate.h"
#import "TMAvSessionManagerFactory.h"
#import "TMMotionCapManagerFactory.h"
#import "TMVideoCapManagerFactory.h"
#import "TMLocationManagerFactory.h"
#import "TMDataManagerFactory.h"

@protocol ModalViewDelegate

- (void)didDismissModalView;

@end

@class TMAppDelegate;

@interface TMHistoryVC : UITableViewController <NSFetchedResultsControllerDelegate, ModalViewDelegate>
{
    NSNumber *unitsPref;
    NSNumber *fractionalPref;
    
    NSFetchedResultsController *fetchedResultsController;
    NSManagedObjectContext *managedObjectContext;
    NSArray *measurementsData;
}

@end
