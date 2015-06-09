#define _USE_MATH_DEFINES

#include "rc_intel_interface.h"
#include "TrackerManager.h"

#include <iostream>
#include <fstream>
#include <string.h>
#include <assert.h>
#include <sstream>
#include <memory>
#include <conio.h>
#include "libpxcimu_internal.h"
#include "pxcsensemanager.h"
#include "stats.h"
#include "rc_pxc_util.h"
#include "RCFactory.h"
#include <thread>
#include <chrono>

using namespace std;
using namespace RealityCap;

void logger(void * handle, const char * buffer_utf8, size_t length)
{
    fprintf((FILE *)handle, "%s\n", buffer_utf8);
}

int wmain(int c, wchar_t **v)
{
    if(c < 2)
    {
        fprintf(stderr, "specify file name\n");
        return 0;
    }

    RCFactory factory;
    auto trackMan = factory.CreateTrackerManager();
    trackMan->SetLog(logger, stdout);

    if (!trackMan->StartReplay(v[1], false))
    {
        fprintf(stderr, "Failed to start replay.\n");
        return 0;
    }
    
    while (trackMan->isVideoStreaming())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    wcout << L"Exiting" << endl;;

    return 0;

}
