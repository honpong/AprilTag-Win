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
#import "RCCore/RCAVSessionManagerFactory.h"
#import "RCCore/RCMotionCapManagerFactory.h"
#import "RCCore/RCVideoCapManagerFactory.h"
#import "RCCore/RCLocationManagerFactory.h"
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
