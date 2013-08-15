//
//  main.m
//  MeasuredPhoto
//
//  Created by Ben Hirashima on 7/26/13.
//  Copyright (c) 2013 RealityCap. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "MPAppDelegate.h"

#warning "Remove the console log hack!"
//TODO: This is a temporary hack for xcode 5 / ios 7 beta printing infinite assertmacro logs to the console
static int (*_oldStdWrite)(void *, const char *, int);

int __pyStderrWrite(void *inFD, const char *buffer, int size)
{
    if ( strncmp(buffer, "AssertMacros:", 13) == 0 ) {
        return size;
    }
    return _oldStdWrite(inFD, buffer, size);
}

int main(int argc, char *argv[])
{
    _oldStdWrite = stderr->_write;
    stderr->_write = __pyStderrWrite;
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([MPAppDelegate class]));
    }
}
