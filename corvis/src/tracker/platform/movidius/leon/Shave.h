
#ifndef __SHAVE_H__
#define __SHAVE_H__

#include <mutex>
#include <OsDrvSvu.h>
#include <mv_types.h>
#define SHAVES_CNT 	(12)

class Shave {

protected:
    Shave(unsigned int a);
    u32 reset(void);
    u32 set_default_stack(void);
    static Shave* shaves[SHAVES_CNT];
    static osDrvSvuHandler_t handlers[SHAVES_CNT];
    static bool drv_initialized;
    unsigned int id;
    unsigned int power_mask;
    std::mutex shave_mutex;

public:
    static Shave* get_handle(unsigned int a);
    static void destroy(unsigned int a);
    void start(unsigned int ptr, const char* fmt, ...);
    void wait(void);
};

#endif
