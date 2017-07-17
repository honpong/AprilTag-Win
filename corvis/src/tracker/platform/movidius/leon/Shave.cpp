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
#include <mv_types.h>
#include <swcShaveLoader.h>
#include <HglCpr.h>
#include "Shave.h" 

Shave::Shave(unsigned int a)
{
    //printf("Shave constructur called for id:%d at 0x%x\n", a, this);
    this->id = a;
    power_mask = 1 << id;
}

Shave* Shave::handles[] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
        NULL, NULL, NULL, NULL };

Shave* Shave::get_handle(unsigned int a)
{
    Shave *obj;

    if (a > SHAVES_CNT) {
        printf("Error: invalid shave ID\n");
        return NULL;
    }

    if (handles[a] == NULL) {
        obj = new Shave(a);
        handles[a] = obj;
        return obj;
    } else {
        return handles[a];
    }
}

void Shave::destroy(unsigned int a)
{
    if (a > SHAVES_CNT) {
        printf("Error: invalid shave ID\n");
        return;
    }

    if (handles[a] != NULL) {
        delete handles[a];
        handles[a] = NULL;
    }
}

void Shave::reset(void)
{
    swcResetShave(id);
}

void Shave::set_default_stack(void)
{
    swcSetAbsoluteDefaultStack(this->id);
}

void Shave::start(unsigned int ptr, const char* fmt, ... )
{
    shave_mutex.lock();
    HglCprTurnOnShaveMask(power_mask);
    this->reset();
    this->set_default_stack();
    va_list a_list;
    va_start(a_list, fmt);
    swcSetRegsCC(id, fmt, a_list);
    va_end(a_list);
    swcStartShave(id, ptr);
}

void Shave::wait(void)
{
    swcWaitShave(id);
    HglCprTurnOnShaveMask(power_mask);
    shave_mutex.unlock();
}
