///
/// @file
/// @copyright All code copyright Movidius Ltd 2012, all rights reserved.
///            For License Warranty see: common/license.txt
///
/// @brief     shave class file 
///

// 1: Includes
// ----------------------------------------------------------------------------

#include <stdio.h>
#include <OsDrvCpr.h>
#include "Shave.h" 
#include "Trace.h"

#define SHAVE_DEBUG_PRINTS
#ifdef SHAVE_DEBUG_PRINTS
    #define DPRINTF(...)  printf(__VA_ARGS__);
#else
    #define DPRINTF(...)
#endif


Shave* Shave::shaves[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL };

osDrvSvuHandler_t Shave::handlers[SHAVES_CNT];

bool Shave::drv_initialized = false;

Shave::Shave(unsigned int a)
{
    u32 sc = OS_MYR_DRV_SUCCESS;
    if(!drv_initialized) {
        sc = OsDrvSvuInit();
        drv_initialized = true;
        if (OS_MYR_DRV_SUCCESS != sc) {
            DPRINTF("Error: failed initializing drv %lu\n", sc);
        }
    }
    this->id = a;
    power_mask = 1 << id;
    sc = OsDrvSvuOpenShave(&handlers[a], a, OS_MYR_PROTECTION_SEM );
    if (OS_MYR_DRV_SUCCESS != sc) {
        DPRINTF("Error: failed creating shave %lu\n", sc);
    }
}

Shave* Shave::get_handle(unsigned int a)
{
    Shave* obj;

    if (a >= SHAVES_CNT) {
        DPRINTF("Error: invalid shave ID\n");
        return NULL;
    }

    if (shaves[a] == NULL) {
        obj = new Shave(a);
        shaves[a] = obj;
        return obj;
    } else {
        return shaves[a];
    }
}

void Shave::destroy(unsigned int a)
{
    if (a > SHAVES_CNT) {
        DPRINTF("Error: invalid shave ID\n");
        return;
    }

    if (shaves[a] != NULL) {
        delete shaves[a];
        shaves[a] = NULL;
    }
}

u32 Shave::reset(void)
{
    return OsDrvSvuResetShave(&handlers[id]);
}

u32 Shave::set_default_stack(void)
{
    return OsDrvSvuSetAbsoluteDefaultStack(&handlers[id]);
}

void Shave::start(unsigned int ptr, const char* fmt, ... )
{
    va_list a_list;
    va_start(a_list, fmt);
    vstart(this, ptr, fmt, a_list);
    va_end(a_list);
}

void shave_start(unsigned int i, unsigned int ptr, const char* fmt, ...)
{
    va_list a_list;
    va_start(a_list, fmt);
    Shave::vstart(Shave::get_handle(i), ptr, fmt, a_list);
    va_end(a_list);
}

void Shave::vstart(Shave *s, unsigned int ptr, const char* fmt, va_list a_list)
{
    u32 sc = OS_MYR_DRV_SUCCESS;
    s->shave_mutex.lock();
#ifndef DISABLE_SHAVE_PM
    sc = OsDrvCprTurnOnShaveMask(s->power_mask);
    if (OS_MYR_DRV_SUCCESS != sc) {
        DPRINTF("Error: failed Turn on shave %lu\n", sc);
    }
#endif
    sc = s->reset();
    if (OS_MYR_DRV_SUCCESS != sc) {
        DPRINTF("Error: failed reset shave %lu\n", sc);
    }
    sc= s->set_default_stack();
    if (OS_MYR_DRV_SUCCESS != sc) {
        DPRINTF("Error: failed setting shave stack %lu\n", sc);
    }
    START_EVENT(EV_SHAVE0 + s->id, 0);
    sc = OsDrvSvuStartShaveCC2(&handlers[s->id], ptr, fmt, a_list);
    if (OS_MYR_DRV_SUCCESS != sc) {
        DPRINTF("Error: failed starting shave %lu\n", sc);
    }
}

void Shave::wait(void)
{
    u32 running = 0, sc = OS_MYR_DRV_SUCCESS;
    sc = OsDrvSvuWaitShaves(1, &handlers[id], OS_DRV_SVU_WAIT_FOREVER, &running);
#ifndef DISABLE_SHAVE_PM
    sc = OsDrvCprTurnOffShaveMask (power_mask);
    if (OS_MYR_DRV_SUCCESS != sc) {
        DPRINTF("Error: failed Turn off shave %lu\n", sc);
    }
#endif
    shave_mutex.unlock();
    if (OS_MYR_DRV_SUCCESS != sc) {
        DPRINTF("Error: while waiting shave %lu\n", sc);
    }
    if (0 != running) {
        DPRINTF("Error: wait shave %lu is running\n", running);
    }
    END_EVENT(EV_SHAVE0 + id, 0);
}

void shave_wait(unsigned int i)
{
    Shave::get_handle(i)->wait();
}
