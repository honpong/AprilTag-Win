//
//  TMHistoryVC.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import "TMMeasurement.h"
#import "TMAppDelegate.h"
#import "TMResultsVC.h"
#import "RCCore/RCDistanceFormatter.h"
#import "TMAppDelegate.h"
#import "RCCore/RCAVSessionManagerFactory.h"
#import "RCCore/RCMotionCapManagerFactory.h"
#import "RCCore/RCVideoCapManagerFactory.h"
#import "RCCore/RCLocationManagerFactory.h"
#import "TMDataManagerFactory.h"
#import "TMMeasurement+TMMeasurementExt.h"
#import "TMMeasurement+TMMeasurementSync.h"
#import "TMSyncable+TMSyncableSync.h"
#import "MBProgressHUD.h"
#import "TMServerOpsFactory.h"

@protocol ModalViewDelegate

- (void)didDismissModalView;

@end

@interface TMHistoryVC : UITableViewController <NSFetchedResultsControllerDelegate, ModalViewDelegate, UIActionSheetDelegate>
{
    NSNumber *unitsPref;
    NSNumber *fractionalPref;
    
    NSFetchedResultsController *fetchedResultsController;
    NSManagedObjectContext *managedObjectContext;
    NSArray *measurementsData;
    
    UIActionSheet *actionSheet;
}

@property (weak, nonatomic) IBOutlet UIBarButtonItem *actionButton;

- (IBAction)handleActionButton:(id)sender;

@end
