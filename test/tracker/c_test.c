#include "rc_tracker.h"

int main(int c, char **v)
{
    rc_destroy(rc_create());
    return 0;
}
