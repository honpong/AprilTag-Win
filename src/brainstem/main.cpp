#include "BrainStem2/BrainStem-all.h"
#include <sstream>
#include <vector>
#include <iostream>


int main(int c, const char *v[]) {
    if (0) { usage:
        std::cerr << "Usage: " << v[0] << " [--enableport <port>{,<port>}...] [--disableport <port>{,<port>}...] [--delay <ms>]\n";
        return 1;
    }

    int delay = 0, port; char delim; std::vector<int> enable, disable;
    for (int i=1; i<c; i++)
        if      (strcmp(v[i], "--enableport")  == 0 && i+1 < c) for (std::stringstream ss(v[++i]); ss>>port; ss>>delim)  enable.push_back(port);
        else if (strcmp(v[i], "--disableport") == 0 && i+1 < c) for (std::stringstream ss(v[++i]); ss>>port; ss>>delim) disable.push_back(port);
        else if (strcmp(v[i], "--delay") == 0 && i+1 < c) delay = std::atoi(v[++i]);
        else goto usage;

    if (enable.size() == 0 && disable.size() == 0)
        goto usage;

    aUSBHub3p stem;
    if (aErr err = stem.discoverAndConnect(USB)) {
        std::cerr << v[0] << ": discover and connect failed: " << aError_GetErrorText(err) << "\n";
        return 1;
    }

    USBClass usb;
    usb.init(&stem,0);

    for (auto port : disable)
        if (aErr err = stem.usb.setPortDisable(port)) {
            std::cerr << v[0] << ": failed to disable port " << port << ": " << aError_GetErrorText(err) << "\n";
            return 1;
        }

    aTime_MSSleep(delay);

    for (auto port : enable)
        if (aErr err = stem.usb.setPortEnable(port)) {
            std::cerr << v[0] << ": failed to enable port " << port << ": " << aError_GetErrorText(err) << "\n";
            return 1;
        }

    aTime_MSSleep(delay);

    stem.disconnect();
    return 0;
}


