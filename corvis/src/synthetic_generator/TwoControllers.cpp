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

unsigned int CBannerTwoControllersScenario::InitializeSetup(std::map<std::string, std::string> args)
{
    unsigned int uRes = 0;
    m_args = args;
    bool isMarkerOn = true;
    if (m_args.find("--secondmarkeroff") != m_args.end())
    {
        cout << "Marker for second controller disabled." << endl;
        isMarkerOn = false;
    }

    uRes = m_spBannerMainScenario->InitializeSetup(m_args);
    if (0 != uRes)
    {
        return uRes;
    }

    if (m_spBannerMainScenario->isControllerAnimated())
    {
        vtkSmartPointer<vtkCoordinate> spOrbCenterCoordinates = vtkSmartPointer<vtkCoordinate>::New();
        vtkSmartPointer<vtkCoordinate> spLEDCenterCoordinates = vtkSmartPointer<vtkCoordinate>::New();
        vtkSmartPointer<vtkCoordinate> spBodyCenterCoordinates = vtkSmartPointer<vtkCoordinate>::New();
        m_spBannerMainScenario->GetControllerActorsCenters(&spOrbCenterCoordinates, &spLEDCenterCoordinates, &spBodyCenterCoordinates, 1);
        m_spBannerMainScenario->AddControllerInWindow(spOrbCenterCoordinates, spLEDCenterCoordinates, spBodyCenterCoordinates, isMarkerOn);
    }
    return uRes;
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
