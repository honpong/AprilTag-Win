//
//  ReplayViewController.m
//  RCCapture
//
//  Created by Brian on 2/18/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

#import "ReplayViewController.h"
#import "RC3DK.h"
#import "CaptureFilename.h"

@interface ReplayViewController ()
{
    RCReplayManager * controller;
    BOOL isStarted;
    NSString * replayFilename;
    NSArray * replayFilenames;
}
@end

@implementation ReplayViewController

@synthesize startButton, progressBar, progressText, outputLabel;

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
        // Custom initialization
    }
    return self;
}

- (void) startReplay
{
    NSLog(@"Start replay");
    BOOL isRealtime = [_realtimeSwitch isOn];
    NSDictionary * parameters = [CaptureFilename parseFilename:replayFilename];
    [controller setupWithPath:replayFilename withRealtime:isRealtime
                    withWidth:[(NSNumber *)[parameters objectForKey:@"width"] intValue]
                   withHeight:[(NSNumber *)[parameters objectForKey:@"height"] intValue]
                withFramerate:[(NSNumber *)[parameters objectForKey:@"framerate"] intValue]];
    [controller startReplay];
    [startButton setTitle:@"Stop" forState:UIControlStateNormal];
    isStarted = TRUE;
}

- (void) stopReplay
{
    NSLog(@"Stop replay");
    [controller stopReplay];
    [startButton setTitle:@"Start" forState:UIControlStateNormal];
    isStarted = FALSE;
}

- (IBAction) startStopClicked:(id)sender
{
    if(isStarted)
        [self stopReplay];
    else
        [self startReplay];
}

- (void)setProgressPercentage:(float)progress
{
    float percent = progress*100.;
    [progressBar setProgress:progress];
    [progressText setText:[NSString stringWithFormat:@"%.0f%%", percent]];
}

- (void) replayProgress:(float)progress
{
    [self setProgressPercentage:progress];
}

- (void) replayFinishedWithLength:(float)length withPathLength:(float)pathLength
{
    NSLog(@"Replay finished");
    [outputLabel setText:[NSString stringWithFormat:@"Straight line length: %f cm\nPath length: %f cm", length, pathLength]];
    [self stopReplay];
}

- (void)initReplayFilenames
{
    NSArray * paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString * documentsDirectory = [paths objectAtIndex:0];
    NSArray * documents = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:documentsDirectory error:NULL];
    NSMutableArray * files = [NSMutableArray array];
    for(NSString * doc in documents) {
        if([doc rangeOfString:@"capture"].location != NSNotFound || [doc rangeOfString:@"_L"].location != NSNotFound) {
            [files addObject:doc];
        }
    }
    replayFilenames = [NSArray arrayWithArray:files];
}

- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view.
    [self setProgressPercentage:0];
    controller = [[RCReplayManager alloc] init];
    controller.delegate = self;
	// Do any additional setup after loading the view.

    [self initReplayFilenames];
    if(replayFilenames.count > 0) {
    NSIndexPath * firstRow = [NSIndexPath indexPathForRow:0 inSection:0];
        [_replayFilesTableView selectRowAtIndexPath:firstRow animated:FALSE scrollPosition:UITableViewScrollPositionNone];
        [self tableView:_replayFilesTableView didSelectRowAtIndexPath:firstRow];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return [replayFilenames count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *simpleTableIdentifier = @"SimpleTableItem";

    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:simpleTableIdentifier];

    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:simpleTableIdentifier];
    }

    cell.textLabel.text = [replayFilenames objectAtIndex:indexPath.row];
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    NSInteger item =[indexPath indexAtPosition:1];
    NSArray * paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString * documentsDirectory = [paths objectAtIndex:0];
    replayFilename = [documentsDirectory stringByAppendingPathComponent:[replayFilenames objectAtIndex:item]];
}

@end
