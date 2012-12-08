// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include <Python.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE /* setresgid/setresuid */
#endif
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
//#include <gtk/gtk.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <signal.h>
#include <pthread.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/mman.h>
#include <dlfcn.h>
#ifndef __APPLE__
#include <linux/unistd.h>
#endif
#include "cor.h"

#ifdef __APPLE__
#import <Cocoa/Cocoa.h>
#endif

bool realtime_mode = true;

#include "sigint.h"
static pthread_t mainthread;

static void main_stop(void * ignore)
{
}
#include <assert.h>

void pythonshell_init() {
}

void * pythonshell_run(void *ignore) {
    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    PyRun_SimpleString(
                       "from IPython.frontend.terminal.embed import InteractiveShellEmbed\n"
                       "ipshell = InteractiveShellEmbed()\n"
                       );
    PyRun_SimpleString("ipshell()\n");
    PyGILState_Release(gstate);
    return NULL;
}

void main_init() {
    pythonshell_init();
}

void main_func() {
    pthread_t shellthread;
    pthread_create(&shellthread, NULL, pythonshell_run, NULL);

    PyGILState_STATE gstate;
    gstate = PyGILState_Ensure();
    PyRun_SimpleString("myvis.app.MainLoop()()\n");
    PyGILState_Release(gstate);

    pthread_join(shellthread, NULL);
}

//for the raw python interface
void init_cor();
void init_corpp();

#ifdef __APPLE__
@interface CocoaMultithreading : NSObject
+ (void)beginMultithreading;
@end

@implementation CocoaMultithreading
+ (void)dummyThread:(id)unused
{
    (void)unused;
}

+ (void)beginMultithreading
{
    [NSThread detachNewThreadSelector:@selector(dummyThread:)
            toTarget:self withObject:nil];
}
@end
#endif

int main(int argc, char **argv, char **e)
{
#ifdef __APPLE__
    [NSAutoreleasePool new];
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    [CocoaMultithreading beginMultithreading];
    //    id menubar = [[NSMenu new] autorelease];
    //id appMenuItem = [[NSMenuItem new] autorelease];
    //[menubar addItem:appMenuItem];
    //[NSApp setMainMenu:menubar];
    //id appMenu = [[NSMenu new] autorelease];
    //id appName = [[NSProcessInfo processInfo] processName];
    //id quitTitle = [@"Quit " stringByAppendingString:appName];
    //id quitMenuItem = [[[NSMenuItem alloc] initWithTitle:quitTitle
    //    action:@selector(terminate:) keyEquivalent:@"q"] autorelease];
    //[appMenu addItem:quitMenuItem];
    //[appMenuItem setSubmenu:appMenu];
    //id window = [[[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 200, 200)
    //    styleMask:NSTitledWindowMask backing:NSBackingStoreBuffered defer:NO]
    //        autorelease];
    //[window cascadeTopLeftFromPoint:NSMakePoint(20,20)];
    //[window setTitle:appName];
    //[window makeKeyAndOrderFront:nil];
    //[NSApp activateIgnoringOtherApps:YES];
    //[Nsapp run];
#endif

    // block sigint for all threads (must be called before any are created)
    sigint_init();

    mainthread = pthread_self();

    //Initialize python
    Py_Initialize();
    PySys_SetArgv(0, argv);

    //initialize the cor swig interface
    init_cor();

    main_init();

    PySys_SetArgv(argc, argv);

    //handle the command line parameters and config scripts
    //no threads yet, so no need for GIL
    FILE *init_py;
    assert((init_py = fopen("init.py", "rt")));
    PyRun_SimpleFile(init_py, "init.py");
    fclose(init_py);

    //this should be handled by python
    int res = 0;
#ifndef __APPLE__
    struct sched_param schp = { .sched_priority = 5 };
    res = sched_setscheduler(0, SCHED_FIFO, &schp);
#endif
    //pthread_setschedparam(mainthread, SCHED_FIFO, &schp);
    if(res) {
        fprintf(stderr, "**********************ALERT**********************\n");
        fprintf(stderr, "Unable to get realtime scheduling priority.\nThis is BAD if you are trying to capture data.\nTo enable realtime scheduling, add yourself to rtprio in /etc/security/limits.conf\n");
    }
    pthread_cleanup_push((void (*)(void *))plugins_stop, NULL);

    //Init python threads and release GIL since there's no python in main (TODO: double-check that we automatically acquire GIL on init)
    PyEval_InitThreads();
    PyGILState_STATE gstate;
    PyGILState_Release(gstate);

    cor_time_init();
    plugins_start();

    pthread_cleanup_push(main_stop, NULL);

    main_func();

    pthread_cleanup_pop(1); // main
    pthread_cleanup_pop(1); // plugins
}
