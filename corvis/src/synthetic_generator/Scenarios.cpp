#include <thread>
#include <mutex>
#include "Utils.h"
#include <new>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include "Scenarios.h"

uint8_t IVTKScenario::LoadCalibrationFile(std::map<std::string, std::string> args)
{
    uint8_t uRes = 0;
    if (args.find("--calibrationfile") != args.end())
    {
        m_spProxy = std::make_unique<TrackerProxy>();
        if (0 != m_spProxy->ReadCalibrationFile(args["--calibrationfile"], &m_szCalibrationContents))
        {
            uRes = 1;
            std::cout << "Reading calibration file failed." << std::endl;
            return uRes;
        }
        for (const auto& i : m_spProxy->getCamerasIntrincs())
        {
            std::shared_ptr<CFakeImgCapturer> spCapturer;
            if (rc_CALIBRATION_TYPE_KANNALA_BRANDT4 == i.cameraintrinsics.type)
            {
                spCapturer = std::make_shared<KB4Capturer>();
            }
            else
            {
                spCapturer = std::make_shared<FOVCapturer>();
            }
            spCapturer->InitializeCapturer();
            spCapturer->SetCameraIntrinsics(i);
            spCapturer->fisheyeRenderSizeCompute();
            m_spCapturers.push_back(spCapturer);
        }
            
        if ((!m_spColorCapturer) || (!m_spDepthCapturer) || (!m_spLFisheyeCapturer) || (!m_spRFisheyeCapturer))
        {
            for (const auto& i : m_spCapturers)
            {
                if (rc_FORMAT_DEPTH16 == i->ImageFormat)
                {
                    m_spColorCapturer = i;
                    m_spDepthCapturer = i;
                }
                else
                {
                    m_spColorCapturer = !m_spColorCapturer ? i : m_spColorCapturer;
                    if (0 == i->CameraIndex)
                    {
                        m_spLFisheyeCapturer = i;
                    }
                    else
                    {
                        m_spRFisheyeCapturer = i;
                    }
                }
            }
        }
        if (!m_spColorCapturer)
        {
            std::cout << "No camera seems to have been specified in the specified calibration file." << std::endl;
            uRes = 2;
            return uRes;
        }
    }
    else
    {
        std::cout << "Please check that the calibration file was specified with --calibrationfile switch and that it was accessible." << std::endl;
        uRes = 1;
        return uRes;
    }
    return uRes;
}

void IVTKScenario::SetupActorsInWindows(vtkRenderer* const &pRenderer, vtkRenderWindow* const & pWindow)
{
    vtkSmartPointer<vtkRenderWindowInteractor> spRenderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    spRenderWindowInteractor->SetRenderWindow(pWindow);
    m_spInteractors.push_back(spRenderWindowInteractor);

    //Create a mapper and actor
    if (!m_spMapper)
    {
        m_spMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
        m_spMapper->SetInputConnection(m_spMesh.get()->m_spNorms->GetOutputPort());
    }
    vtkSmartPointer<vtkActor> spActor = vtkSmartPointer<vtkActor>::New();
    // load the texture
    if (m_args.find("--texture") != m_args.end())
    {
        if (!m_spTexture)
        {
            m_spTexture = vtkSmartPointer<vtkTexture>::New();
            vtkSmartPointer<vtkImageReader2Factory> readerFactory = vtkSmartPointer<vtkImageReader2Factory>::New();
            vtkSmartPointer<vtkImageReader2> spImageReader = vtkSmartPointer<vtkImageReader2>::New();
            spImageReader = readerFactory->CreateImageReader2(m_args["--texture"].c_str());
            spImageReader->SetFileName(m_args["--texture"].c_str());
            spImageReader->Update();
            m_spTexture->SetInputConnection(spImageReader->GetOutputPort());
            m_spTexture->SetQualityToDefault();
            m_spTexture->InterpolateOn();
            m_spTexture->Update();
        }
    }
    spActor->SetMapper(m_spMapper);
    spActor->SetTexture(m_spTexture);
    spActor->Modified();
    spActor->SetScale(1.0 / 100, 1.0 / 100, 1.0 / 100);
    spActor->GetProperty()->BackfaceCullingOff();
    spActor->GetProperty()->FrontfaceCullingOff();
    pRenderer->AddActor(spActor);
}

