//
//  AppDelegate.h
//  RC3DKSampleVis
//
//  Created by Brian on 8/26/13.
//  Copyright (c) 2013 Realitycap. All rights reserved.
//

#import <Cocoa/Cocoa.h>

#import "RCOpenGLView.h"

@class GCDAsyncSocket;

@interface AppDelegate : NSObject <NSApplicationDelegate, NSNetServiceDelegate>
{
    GCDAsyncSocket *listenSocket;
    GCDAsyncSocket *connectedSocket;
    NSNetService *netService;
}
@property (assign) IBOutlet NSWindow *window;
@property (assign) IBOutlet RCOpenGLView *glview;

@end
