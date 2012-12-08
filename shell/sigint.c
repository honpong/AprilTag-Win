// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#include <pthread.h>
#include <signal.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <pthread.h>
#include "sigint.h"

#include "cor.h"

static sigset_t sigs;

void sigint_init()
{
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigs, NULL);
}

void sigint_handler()
{
    int sig;
    while(1) {
        int res = sigwait(&sigs, &sig);
        if(res == 0) {
            return;
        } else if (res != EINTR) {
            fprintf(stderr, "cor: sigint failed on sigwait: %s\n", strerror(res));
            exit(EXIT_FAILURE);
        }
        pthread_testcancel();
    }
}

