#include <thread>
#include <mutex>
#include "Utils.h"
#include <new>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include "Scenarios.h"

CBannerTwoControllersScenario::CBannerTwoControllersScenario()
{
    m_spBannerMainScenario = std::make_shared<CBannerSimpleScenario>();
}

size_t CBannerTwoControllersScenario::InitializeSetup(std::map<std::string, std::string> args)
{
    size_t res = 0;
    m_args = args;

	res = m_spBannerMainScenario->InitializeSetup(m_args);
    if (0 != res)
    {
        return res;
    }

    if (m_spBannerMainScenario->isControllerAnimated())
    {
		double controller2x = 0.002f, controller2y = 0.009f, controller2z = 0.028f;
        vtkSmartPointer<vtkCoordinate> spControllerCenterCoordinates = vtkSmartPointer<vtkCoordinate>::New();
		if (m_args.end() != m_args.find("--controller2"))
		{
			size_t end = 0;
			const char * s = m_args.at("--controller2").c_str();
			controller2x = std::stod(s, &end); // the +1s below skip the user's delimiter
			s += end + 1;
			controller2y = std::stod(s, &end);
			s += end + 1;
			controller2z = std::stod(s, &end);
		}
		std::cout << std::fixed << setw(12) << std::setprecision(8) << "Controller2 initial position:(" << controller2x << "," << controller2y << "," << controller2z << ")" << std::endl;
		spControllerCenterCoordinates->SetValue(controller2x, controller2y, controller2z);
        m_spBannerMainScenario->AddControllerInWindow(spControllerCenterCoordinates,1/*Controller index in window*/);
    }
    return res;
}

void CBannerTwoControllersScenario::Execute()
{
    if (m_spBannerMainScenario->isAnimationEnabled())
    {
        vtkSmartPointer<vtkTransformInterpolator> spNewInterpolation = vtkSmartPointer<vtkTransformInterpolator>::New();
        if (m_spBannerMainScenario->isControllerAnimated())
        {
            m_spBannerMainScenario->GenerateInterpolations(m_args["--secondcontrolleranimation"].c_str(), &spNewInterpolation);
        }
        m_spBannerMainScenario->AddNewControllerInterpolation(spNewInterpolation);
        m_spBannerMainScenario->CreateCamerasCallbacks();
        m_spBannerMainScenario->CreateTimerWindowCallback();
        m_spBannerMainScenario->StartScenario();
    }
    else
    {
        PrintUsage();
    }
}
