///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     RTEMS configuration Leon header
///
#ifndef _RTEMS_CONFIG_H_
#define _RTEMS_CONFIG_H_

#if defined(__RTEMS__)

#if !defined(__CONFIG__)
#define __CONFIG__

/* ask the system to generate a configuration table */
#define CONFIGURE_INIT

#ifndef RTEMS_POSIX_API
#define RTEMS_POSIX_API
#endif

#define CONFIGURE_MICROSECONDS_PER_TICK 1000 /* 1 millisecond */
#define CONFIGURE_TICKS_PER_TIMESLICE 10     /* 10 milliseconds */

#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER

#define CONFIGURE_MINIMUM_TASK_STACK_SIZE (8 * 1024)

#define CONFIGURE_POSIX_INIT_TASKS_TABLE
#define CONFIGURE_POSIX_INIT_THREAD_TABLE

#define CONFIGURE_UNIFIED_WORK_AREAS
#define CONFIGURE_UNLIMITED_OBJECTS

#define CONFIGURE_MAXIMUM_TASKS 16
#define CONFIGURE_MAXIMUM_SEMAPHORES 24
#define CONFIGURE_MAXIMUM_TIMERS 16
#define CONFIGURE_MAXIMUM_MESSAGE_QUEUES 20
#define CONFIGURE_MAXIMUM_DRIVERS 32
#define CONFIGURE_MAXIMUM_DEVICES 32

#define CONFIGURE_MAXIMUM_POSIX_THREADS 32
#define CONFIGURE_MAXIMUM_POSIX_MUTEXES 16
#define CONFIGURE_MAXIMUM_POSIX_KEYS 16
#define CONFIGURE_MAXIMUM_POSIX_SEMAPHORES 32
#define CONFIGURE_MAXIMUM_POSIX_MESSAGE_QUEUES 16
#define CONFIGURE_MAXIMUM_POSIX_TIMERS 16
#define CONFIGURE_MAXIMUM_POSIX_CONDITION_VARIABLES 16

#define CONFIGURE_LIBIO_MAXIMUM_FILE_DESCRIPTORS 64

static void Fatal_extension(Internal_errors_Source the_source, bool is_internal,
                            uint32_t the_error);

void POSIX_Init(void * args);

#define CONFIGURE_MAXIMUM_USER_EXTENSIONS 1
#define CONFIGURE_INITIAL_EXTENSIONS \
    {                                \
        .fatal = Fatal_extension     \
    }

#include <rtems/confdefs.h>

#endif  // __CONFIG__
#endif  // __RTEMS__

#endif  // _RTEMS_CONFIG_H_
