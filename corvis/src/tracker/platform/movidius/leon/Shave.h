
#ifndef __SHAVE_H__
#define __SHAVE_H__

#include <mutex>
#define SHAVES_CNT 	(12)

class Shave {

private:
    Shave(unsigned int a);
    static Shave *handles[SHAVES_CNT];
    unsigned int id;
    unsigned int power_mask;
    std::mutex shave_mutex;

public:
    static Shave *get_handle(unsigned int a);
    static void destroy(unsigned int a);
    void reset(void);
    void set_default_stack(void);
    void start(unsigned int ptr, const char* fmt, ...);
    void wait(void);
};

#endif
