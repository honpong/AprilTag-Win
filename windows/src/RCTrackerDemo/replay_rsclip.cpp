#include "rc_intel_interface.h"
#include "TrackerManager.h"
#include "RCFactory.h"
#include <iostream>

using namespace RealityCap;

void logger(void * handle, const char * buffer_utf8, size_t length)
{
    fprintf((FILE *)handle, "%s\n", buffer_utf8);
}

int wmain(int c, wchar_t **v)
{
    if(c != 3)
    {
        fprintf(stderr, "usage: %ws <rssdk-clip> <rc-capture-out>\n", v[0]);
        return 0;
    }

    RCFactory factory;
    auto trackMan = factory.CreateTrackerManager();
    trackMan->SetLog(logger, stdout);
    if(c == 3) {
        trackMan->SetOutputLog(std::wstring(v[2]));
    }

    if (!trackMan->StartReplay(v[1], false))
    {
        fprintf(stderr, "Failed to start replay.\n");
        return 0;
    }

    trackMan->WaitUntilFinished();

    std::cerr << trackMan->getTimingStats();

    return 0;

}
