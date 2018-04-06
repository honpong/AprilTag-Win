/********************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
terms of a license agreement or nondisclosure agreement with Intel Corporation
and may not be copied or disclosed except in accordance with the terms of that
agreement.
Copyright(c) 2011-2015 Intel Corporation. All Rights Reserved.
*********************************************************************************/

#include <thread>
#include <mutex>
#include "Utils.h"
#include <new>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include "Callbacks.h"
#include "Scenarios.h"

int main(int argvc, char** ppArgv)
{
    int res = 0;
    std::unique_ptr<IVTKScenario> spScenario;

    std::map<std::string, std::string> args = ParseCmdArgs(argvc, ppArgv);

    if (args.end() == args.find("--mesh"))
    {
        std::cout << "Please specify the mesh file." << std::endl;
        PrintUsage();
        res = 1;
        return res;
    }

    if (args.end() != args.find("--scenario") && "multiplecontrollers" == args["--scenario"])
    {
        spScenario = std::make_unique<CBannerTwoControllersScenario>();
    }
    else if (args.end() != args.find("--scenario") && "interactive" == args["--scenario"])
    {
        spScenario = std::make_unique<CInteractiveScenario>();
    }
    else
    {
        spScenario = std::make_unique<CBannerSimpleScenario>();
    }

    if (0 == (res = spScenario->InitializeSetup(args)))
    {
        spScenario->Execute();
    }

    return res;
}
