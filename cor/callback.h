// Copyright (c) 2008-2012, Eagle Jones
// All rights reserved.
//
// This file is a part of the corvis framework, and is made available
// under the BSD license; please see LICENSE file for full text

#ifndef __CALLBACK_H
#define __CALLBACK_H

#include <stdlib.h>

#define callback_dispatch(type, group, ...) do {                        \
        struct callback_list_##type *client = group;                    \
        while(client) {                                                 \
            client->listener(client->cookie, ##__VA_ARGS__);            \
            client = client->next;                                      \
        }                                                               \
    } while(0)

#define callback_subscribe(type, group, callee, dat) do {               \
        struct callback_list_##type *client = (struct callback_list_##type *)malloc(sizeof(struct callback_list_##type)); \
        client->listener = callee;                                      \
        client->cookie   = dat;                                         \
        client->next = group;                                           \
        group = client;                                                 \
    } while(0)

#define callback_unsubscribe(type, group, dat) do {                     \
        struct callback_list_##type *client = group;                    \
        struct callback_list_##type **prev = &group;                    \
        while(client) {                                                 \
            if(client->cookie == dat) {                                 \
                *prev = client->next;                                   \
                free(client);                                           \
                break;                                                  \
            }                                                           \
            prev = &client->next;                                       \
            client = client->next;                                      \
        }                                                               \
    } while(0)

#define callback_define(type,...)                                       \
    struct callback_list_##type {                                       \
        void (*listener)(void *cookie, ##__VA_ARGS__);                  \
        void *cookie;                                                   \
        struct callback_list_##type *next;                              \
    }

#endif
