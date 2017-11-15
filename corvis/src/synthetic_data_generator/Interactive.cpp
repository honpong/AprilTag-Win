#include <thread>
#include <mutex>
#include "Utils.h"
#include <new>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include "Scenarios.h"

unsigned int CInteractiveScenario::InitializeSetup(std::map<std::string, std::string> args)
{
    unsigned int uRes = 0;

    if (0 != LoadCalibrationFile(args))
    {
        uRes = 1;
        std::cout << "Loading calibration file failed." << std::endl;
        return uRes;
    }

    for (const auto& i : m_spCapturers)
    {
        if (rc_FORMAT_DEPTH16 == i->ImageFormat)
        {
            m_spColorCapturer = i;
            break;
        }
    }

    if (!m_spColorCapturer)
    {
        uRes = 2;
        std::cout << "Interactive mode is to be used with a calibration file with depth/color cameras enabled." << std::endl;
        return uRes;
    }

    m_args = args;
    vtkSmartPointer<vtkCoordinate> spColorPosition = vtkSmartPointer<vtkCoordinate>::New();
    spColorPosition->SetValue(10, 0, 0);
    const RGBColor Background = { 0,0,0 };
    m_spMesh = std::make_shared<CMesh>(m_args);
    m_spMesh->Create();
    m_spColorWindow = std::make_shared<CSimulatedWindow>(m_spColorCapturer->ColorResolution.width, m_spColorCapturer->ColorResolution.height, spColorPosition, m_spColorCapturer->spColorFOV->GetValue()[1/*y*/], 0/* projectionValue*/, Background, 1 /*LightingValue*/, "Window Color");
    m_spColorWindow->Create();
    SetupActorsInWindows(m_spColorWindow->m_spRenderer, m_spColorWindow->m_spRenderWindow);
    m_spCameraPosition = vtkSmartPointer<ModifiedCameraPosition>::New();
    m_spColorWindow->m_spRenderer->GetActiveCamera()->AddObserver(vtkCommand::ModifiedEvent, m_spCameraPosition);
    return uRes;
}

void CInteractiveScenario::Execute()
{
    m_spInteractors[0]->Start();
}
