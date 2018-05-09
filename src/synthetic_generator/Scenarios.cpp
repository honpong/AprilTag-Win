#include <thread>
#include <mutex>
#include "Utils.h"
#include <new>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include "Scenarios.h"

size_t IVTKScenario::LoadCalibrationFile(std::map<std::string, std::string> args)
{
    uint8_t res = 0;
    if (args.find("--calibrationfile") != args.end())
    {
        m_spProxy = std::make_unique<TrackerProxy>();
        if (0 != (res = m_spProxy->ReadCalibrationFile(args["--calibrationfile"], &m_szCalibrationContents)))
        {
            std::cout << "Reading calibration file failed." << std::endl;
            return res;
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
        
        size_t uFisheyeIndex = 0;
        if ((!m_spColorCapturer) || (!m_spDepthCapturer) || (!m_spLFisheyeCapturer) || (!m_spRFisheyeCapturer))
        {
            for (const auto& i : m_spCapturers)
            {
                if (rc_FORMAT_RGBA8 == i->ImageFormat)
                {
                    m_spColorCapturer = i;
                    mkdir((m_szDirectoryName + std::string("/color")).c_str(), 0777);
                    m_spColorCapturer->SetDirectoryName(m_szDirectoryName + std::string("/color/") + std::string(COLOR_RELATIVE_PATH));
                }
                else if (rc_FORMAT_DEPTH16 == i->ImageFormat)
                {
                    m_spDepthCapturer = i;
                    mkdir((m_szDirectoryName + std::string("/depth")).c_str(), 0777);
                    m_spDepthCapturer->SetDirectoryName(m_szDirectoryName + std::string("/depth/") + std::string(DEPTH_RELATIVE_PATH));
                }
                else if(rc_FORMAT_GRAY8 == i->ImageFormat)
                {
                    const std::string szFisheye = std::string("/fisheye") + std::string("_") + std::to_string(uFisheyeIndex) + std::string("/");
                    mkdir((m_szDirectoryName + szFisheye).c_str(), 0777);

                    if (0 == i->CameraIndex)
                    {
                        i->SetDirectoryName(m_szDirectoryName + szFisheye + std::string(LFISHEYE_RELATIVE_PATH));
                        m_spLFisheyeCapturer = i;
                    }
                    else
                    {
                        i->SetDirectoryName(m_szDirectoryName + szFisheye + std::string(RFISHEYE_RELATIVE_PATH));
                        m_spRFisheyeCapturer = i;
                    }
                    uFisheyeIndex++;
                }
            }
            if (!m_spColorCapturer && m_spLFisheyeCapturer)
            {
                m_spColorCapturer = m_spLFisheyeCapturer;
            }
            else if (!m_spColorCapturer && m_spDepthCapturer)
            {
                m_spColorCapturer = m_spDepthCapturer;
            }
        }
    }
    else
    {
        std::cout << "Please check that the calibration file was specified with --calibrationfile switch and that it was accessible." << std::endl;
        res = 1;
        return res;
    }
    return res;
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
