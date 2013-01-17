//
//  TMMeasurementsList.h
//  TapeMeasure
//
//  Created by Ben Hirashima on 11/1/12.
//  Copyright (c) 2012 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreData/CoreData.h>


@protocol ModalViewDelegate

- (void)didDismissModalView;

@end

@class TMAppDelegate;

@interface TMHistoryVC : UITableViewController <NSFetchedResultsControllerDelegate, ModalViewDelegate>
{
    NSNumber *unitsPref;
    NSNumber *fractionalPref;
    
    TMAppDelegate *appDel;
}

- (IBAction)handleDeleteButton:(id)sender;

@property (strong, nonatomic) NSFetchedResultsController *fetchedResultsController;
@property (strong, nonatomic) NSManagedObjectContext *managedObjectContext;
@property (nonatomic, strong) NSArray *measurementsData;

@end
