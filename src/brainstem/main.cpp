
#include "BrainStem2/BrainStem-all.h"
#include <list>
#include <iostream>
#include <map>
#include <string>
#include <algorithm>

aErr DisableEnableCommon(USBClass* const pUsbClass, const std::string& action, std::map<std::string, std::string>& args,bool bEnable)
{
    const char* pPort = args.at(action).c_str();
    size_t port = 0;
    const char* const pEnd = pPort + args.at(action).size();
    size_t delay = 5; 
    size_t end = 0;
    aErr err = aErrNone;

    if(args.end() != args.find("--delay"))
    {
        delay = std::stod(args["--delay"]);
    }

    do
    {
        port = std::stod(pPort,&end);
        err = bEnable ? pUsbClass->setPortEnable(port) : pUsbClass->setPortDisable(port);
        if (aErrNone != err)
        {
            std::cout<< action <<"failed with error:" << err << "and port" << port << std::endl;
        }
        aTime_MSSleep(delay);
        pPort += end;
        if (pPort != pEnd)
        {
            pPort += 1;
        }
    }while(pPort != pEnd);
    return err;
}

int main(int argc, const char * argv[]) {
    aErr err = aErrNone;
    a40PinModule stem;
    USBClass usbclass;
    err = stem.discoverAndConnect(USB);
    if (aErrNone != err) 
    {
        std::cout <<"discoverAndConnect failed with error:" << err << std::endl;
        return err;
    }

    Module* const pModule = dynamic_cast<Module*>(&stem);
    if(!pModule)
    {
        err = aErrNotFound; 
        std::cout <<"Module not found:" << err << std::endl;
    }
    usbclass.init(pModule,0);

    std::map<std::string, std::string> args;
    for (int i = 1; i < argc - 1; i += 2)
    {
        std::string userArg = std::string(argv[i]);
        std::transform(userArg.begin(), userArg.end(), userArg.begin(), ::tolower);
        args.insert(std::pair<std::string, std::string>(userArg, std::string(argv[i + 1])));
    }

    if(args.end() != args.find("--disableport"))
    {
        err = DisableEnableCommon(&usbclass, "--disableport", args,false /*disable*/);
    }

    if(args.end() != args.find("--enableport"))
    {
        err = DisableEnableCommon(&usbclass, "--enableport", args,true/*enable*/);
    }

    // Disconnect from the module
    stem.disconnect();
    return err; 
}


