#include <thread>
#include <mutex>
#include "Utils.h"
#include <new>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include "Scenarios.h"

CBannerSimpleScenario::CBannerSimpleScenario()
{
    m_isRecordingEnabled = false;
    m_isFisheyeStereoRecordingEnabled = false;
    m_isAnimationEnabled = false;
    m_isControllerAnimated = false;
    m_frameIndex = -1;
    m_NumberOfFrames = 0;
    m_RenderingTimeInterval = 0;
}

bool CBannerSimpleScenario::isControllerAnimated() const
{
    return m_isControllerAnimated;
}

bool CBannerSimpleScenario::isAnimationEnabled() const
{
    return m_isAnimationEnabled;
}

void CBannerSimpleScenario::HandleDirectoryCreation()
{
    m_szDirectoryName = std::string("ControllerData");
    if (m_args.find("--directory") != m_args.end())
    {
        m_szDirectoryName = m_args["--directory"];
    }
#ifdef _WIN32
    system((std::string("rd /S /Q ") + m_szDirectoryName).c_str());
#else
    system((std::string("rm -rf ") + m_szDirectoryName).c_str());
#endif
    mkdir(m_szDirectoryName.c_str(), 0777);
}

size_t CBannerSimpleScenario::ParseUserInput(std::map<std::string, std::string> args)
{
    size_t res = 0;
    m_args = args;

    HandleDirectoryCreation();

    if (0 != (res = LoadCalibrationFile(m_args)))
    {
        std::cout << "Load calibration file failed." << std::endl;
        return res;
    }

    m_spMesh = std::make_shared<CMesh>(m_args);
    m_spMesh->Create();

    if (m_args.find("--controlleranimation") != m_args.end())
    {
        m_isControllerAnimated = true;
    }

    if (args.find("--record") != args.end())
    {
        m_isRecordingEnabled = true;
    }

    if (args.find("--fisheyestereo") != args.end())
    {
        m_isFisheyeStereoRecordingEnabled = true;
    }

    if (m_isRecordingEnabled)
    {
        // set files
        char framesFileName[FILE_SIZE_MAX] = { 0 };
        char referenceFileName[FILE_SIZE_MAX] = { 0 };
        char animationFileName[FILE_SIZE_MAX] = { 0 };
        if (0 > snprintf(framesFileName, sizeof(framesFileName), "%s/rgbdepthF.txt", m_szDirectoryName.c_str()))
        {
            cout << "snprintf failed." << "\nLine:" << __LINE__ << "\nFunction:" << __FUNCTION__ << endl;
        }
        m_framesFile.open(framesFileName);
        if (0 > snprintf(referenceFileName, sizeof(referenceFileName), "%s/OCLRefLogFileF.txt", m_szDirectoryName.c_str()))
        {
            cout << "snprintf failed." << "\nLine:" << __LINE__ << "\nFunction:" << __FUNCTION__ << endl;
        }
        m_referenceFile.open(referenceFileName);
        if (0 > snprintf(animationFileName, sizeof(animationFileName), "%s/anim.txt", m_szDirectoryName.c_str()))
        {
            cout << "snprintf failed." << "\nLine:" << __LINE__ << "\nFunction:" << __FUNCTION__ << endl;
        }
        m_animationFile.open(animationFileName);
    }

    if (args.find("--animation") != args.end())
    {
        m_isAnimationEnabled = true;
    }
    else
    {
        std::cout << "Animation file is required for non interactive scenarios." << std::endl;
        PrintUsage();
        res = 2;
        return res;
    }

    if (args.find("--numberofframes") != args.end())
    {
        m_NumberOfFrames = std::stol(args["--numberofframes"].c_str());
    }
    else
    {
        m_NumberOfFrames = DEFAULT_NUMBER_OF_INTERPOLATION_POINTS;
    }

    if (args.find("--renderingtimeinterval") != args.end())
    {
        m_RenderingTimeInterval = std::stol(args["--renderingtimeinterval"].c_str());
    }
    else
    {
        m_RenderingTimeInterval = DEFAULT_RENDERING_TIME_INTERVAL;
    }
    return res;
}

void CBannerSimpleScenario::CreateCameraWindows()
{
    const RGBColor Background = { 0,0,0 };
    m_spRendererCollection = vtkSmartPointer<vtkRendererCollection>::New();

    if (m_spColorCapturer)
    {
        vtkSmartPointer<vtkCoordinate> spColorPosition = vtkSmartPointer<vtkCoordinate>::New();
        spColorPosition->SetValue(10, 0, 0);
        m_spColorWindow = std::make_shared<CSimulatedWindow>(m_spColorCapturer->ColorResolution.width, m_spColorCapturer->ColorResolution.height, spColorPosition, m_spColorCapturer->spColorFOV->GetValue()[1/*y*/], 0/* projectionValue*/, Background, 1 /*LightingValue*/, "Window Color");
        m_spColorWindow->Create();
        m_spSColorWindowCallback = vtkSmartPointer<ColorWindowStartCallback>::New();
        m_spEColorWindowCallback = vtkSmartPointer<ColorWindowEndCallback>::New();
        m_spRendererCollection->AddItem(m_spColorWindow.get()->m_spRenderer);
    }

    if (m_spDepthCapturer)
    {
        vtkSmartPointer<vtkCoordinate> spDepthPosition = vtkSmartPointer<vtkCoordinate>::New();
        spDepthPosition->SetValue(static_cast<double>(m_spDepthCapturer->ColorResolution.width + 10), 0, 0);
        m_spDepthWindow = std::make_shared<CSimulatedWindow>(m_spDepthCapturer->DepthResolution.width, m_spDepthCapturer->DepthResolution.height, spDepthPosition, m_spDepthCapturer->spDepthFOV->GetValue()[1/*y*/], 0/* projectionValue*/, Background, 1 /*LightingValue*/, "Window Depth");
        m_spDepthWindow->Create();
        m_spEDepthWindowCallback = vtkSmartPointer<DepthWindowEndCallback>::New();
        m_spRendererCollection->AddItem(m_spDepthWindow.get()->m_spRenderer);
    }

    if (m_spLFisheyeCapturer)
    {
        vtkSmartPointer<vtkCoordinate> spLFisheyePosition = vtkSmartPointer<vtkCoordinate>::New();
        spLFisheyePosition->SetValue(static_cast<double>(2 * m_spLFisheyeCapturer->ColorResolution.width + 10), 0, 0);
        m_spLFisheyeWindow = std::make_shared<CSimulatedWindow>(m_spLFisheyeCapturer->InputFisheyeResolution.width, m_spLFisheyeCapturer->InputFisheyeResolution.height, spLFisheyePosition, m_spLFisheyeCapturer->spInputFisheyeFOV->GetValue()[1/*y*/], 0/* projectionValue*/, Background, 1 /*LightingValue*/, "Window Fisheye");
        m_spLFisheyeWindow->Create();
        m_spLFisheyeWindow->m_spRenderWindow->SetBorders(0/*Turn on*/);
        m_spLFisheyeWindow.get()->m_spRenderWindow->SetOffScreenRendering(1);
        m_spELFisheyeWindowCallback = vtkSmartPointer<LFisheyeWindowEndCallback>::New();
        m_spRendererCollection->AddItem(m_spLFisheyeWindow.get()->m_spRenderer);
    }

    if (m_spRFisheyeCapturer)
    {
        vtkSmartPointer<vtkCoordinate> spRFisheyePosition = vtkSmartPointer<vtkCoordinate>::New();
        spRFisheyePosition->SetValue(static_cast<double>(3 * m_spRFisheyeCapturer->ColorResolution.width + 10), 0, 0);
        m_spRFisheyeWindow = std::make_shared<CSimulatedWindow>(m_spRFisheyeCapturer->InputFisheyeResolution.width, m_spRFisheyeCapturer->InputFisheyeResolution.height, spRFisheyePosition, m_spRFisheyeCapturer->spInputFisheyeFOV->GetValue()[1/*y*/], 0/* projectionValue*/, Background, 1 /*LightingValue*/, "Window Second Fisheye");
        m_spRFisheyeWindow->Create();
        m_spRFisheyeWindow.get()->m_spRenderWindow->SetBorders(0/*Turn on*/);
        m_spRFisheyeWindow.get()->m_spRenderWindow->SetOffScreenRendering(1);
        m_spERFisheyeWindowCallback = vtkSmartPointer<RFisheyeWindowEndCallback>::New();
        m_spRendererCollection->AddItem(m_spRFisheyeWindow.get()->m_spRenderer);
    }
}

size_t CBannerSimpleScenario::InitializeSetup(std::map<std::string, std::string> args)
{
    size_t res = 0;

    if (0 != (res = ParseUserInput(args)))
    {
        return res;
    }

    CreateCameraWindows();

    if (m_isControllerAnimated)
    {
        double controller1x = 0.002f, controller1y = 0.002f, controller1z = 0.002f;
        m_spActorCollection.push_back(vtkSmartPointer<vtkActorCollection>::New());
        vtkSmartPointer<vtkCoordinate> spControllerCenterCoordinates = vtkSmartPointer<vtkCoordinate>::New();
        if (m_args.end() != m_args.find("--controller1"))
        {
            size_t end = 0;
            const char * s = m_args.at("--controller1").c_str();
            controller1x = std::stod(s, &end); // the +1s below skip the user's delimiter
            s += end + 1;
            controller1y = std::stod(s, &end);
            s += end + 1;
            controller1z = std::stod(s, &end);
        }
        std::cout << std::fixed << setw(12) << std::setprecision(8) << "Controller1 initial position:(" << controller1x << "," << controller1y << "," << controller1z << ")" << std::endl;
        spControllerCenterCoordinates->SetValue(controller1x, controller1y, controller1z);
        if (0 != (res = HandleControllerFile(spControllerCenterCoordinates, 0/*Controller index in window*/)))
        {
            return res;
        }
    }

    if (m_spColorWindow)
    {
        SetupActorsInWindows(m_spColorWindow.get()->m_spRenderer, m_spColorWindow.get()->m_spRenderWindow);
    }

    if (m_spDepthWindow)
    {
        SetupActorsInWindows(m_spDepthWindow.get()->m_spRenderer, m_spDepthWindow.get()->m_spRenderWindow);
    }

    if (m_spLFisheyeWindow)
    {
        SetupActorsInWindows(m_spLFisheyeWindow.get()->m_spRenderer, m_spLFisheyeWindow.get()->m_spRenderWindow);
    }

    if (m_spRFisheyeWindow)
    {
        SetupActorsInWindows(m_spRFisheyeWindow.get()->m_spRenderer, m_spRFisheyeWindow.get()->m_spRenderWindow);
    }

    if (m_isControllerAnimated)
    {
        m_spRendererCollection->InitTraversal();
        for (int k = 0; k < m_spRendererCollection->GetNumberOfItems(); ++k)
        {
            const auto&& pRenderer = m_spRendererCollection->GetNextItem();

            for (uint64_t j = 0; j < static_cast<uint64_t>(m_spActorCollection.size()); ++j)
            {
                m_spActorCollection[j]->InitTraversal();
                vtkActor* const pControllerActor = m_spActorCollection[j]->GetNextActor();
                pControllerActor->GetProperty()->LightingOff();
                pRenderer->AddActor(pControllerActor);
                pRenderer->Modified();
            }
        }
    }

    if (m_spColorCapturer)
    {
        m_spColorWindow.get()->m_spRenderWindow->AddObserver(vtkCommand::StartEvent, m_spSColorWindowCallback);
        m_spColorWindow.get()->m_spRenderWindow->AddObserver(vtkCommand::EndEvent, m_spEColorWindowCallback);
    }

    if(m_spDepthCapturer)
    {
        m_spDepthWindow.get()->m_spRenderWindow->AddObserver(vtkCommand::EndEvent, m_spEDepthWindowCallback);
    }

    if (m_spLFisheyeWindow)
    {
        m_spLFisheyeWindow.get()->m_spRenderWindow->AddObserver(vtkCommand::EndEvent, m_spELFisheyeWindowCallback);
    }

    if (m_spRFisheyeWindow)
    {
        m_spRFisheyeWindow.get()->m_spRenderWindow->AddObserver(vtkCommand::EndEvent, m_spERFisheyeWindowCallback);
    }

    if (m_isControllerAnimated)
    {
        m_spTransformInterpolatorCollection.push_back(vtkSmartPointer<vtkTransformInterpolator>::New());
        GenerateInterpolations(m_args["--controlleranimation"].c_str(), &m_spTransformInterpolatorCollection.back());
    }

    return res;
}

void CBannerSimpleScenario::CreateCamerasCallbacks()
{
    std::shared_ptr<EndCallback::CallbackInput> spEndInput = std::make_shared<EndCallback::CallbackInput>(m_spCapturers, &m_framesFile, &m_referenceFile, &m_animationFile, m_spSColorWindowCallback.GetPointer(), &m_frameIndex, m_NumberOfFrames, &m_isFisheyeStereoRecordingEnabled, &m_isRecordingEnabled,
        &(m_spColorWindow.get()->m_spRenderer), &(m_spDepthWindow.get()->m_spRenderer), &(m_spColorWindow.get()->m_spRenderWindow), &(m_spDepthWindow.get()->m_spRenderWindow),
        &(m_spLFisheyeWindow.get()->m_spRenderWindow), &(m_spRFisheyeWindow.get()->m_spRenderWindow));
    if (m_spEColorWindowCallback)
    {
        m_spEColorWindowCallback.GetPointer()->InitializeInputs(spEndInput);
    }
    if (m_spELFisheyeWindowCallback)
    {
        m_spELFisheyeWindowCallback.GetPointer()->InitializeInputs(spEndInput);
    }
    if (m_spERFisheyeWindowCallback)
    {
        m_spERFisheyeWindowCallback.GetPointer()->InitializeInputs(spEndInput);
    }
    if (m_spEDepthWindowCallback)
    {
        m_spEDepthWindowCallback.GetPointer()->InitializeInputs(spEndInput);
    }

    std::vector<vtkSmartPointer<vtkActor>> spIMUOnController;
    for (size_t i = 0; i < m_spActorCollection.size(); ++i)
    {
        m_spActorCollection[i]->InitTraversal();
        spIMUOnController.push_back(m_spActorCollection[i]->GetNextActor());
    }

    std::shared_ptr<StartCallback::CallbackInput> spStartInput = std::make_shared<StartCallback::CallbackInput>(m_spControllerAnimFile, &m_frameIndex, &m_isControllerAnimated, spIMUOnController,
        &(m_spColorWindow.get()->m_spRenderWindow), &(m_spDepthWindow.get()->m_spRenderWindow), &(m_spLFisheyeWindow.get()->m_spRenderWindow), &(m_spRFisheyeWindow.get()->m_spRenderWindow));
    if (m_spSColorWindowCallback)
    {
        m_spSColorWindowCallback.GetPointer()->InitializeInputs(spStartInput);
    }
}

size_t CBannerSimpleScenario::HandleControllerFile(const vtkSmartPointer<vtkCoordinate>& spOrbCenterCoordinates, uint64_t uIndex)
{
    double scale = 0.00008;
    size_t ret = 0;
    vtkSmartPointer<vtkSTLReader> spControllerFileReader = vtkSmartPointer<vtkSTLReader>::New();
    vtkSmartPointer<vtkActor> spControllerActor = vtkSmartPointer<vtkActor>::New();
    vtkSmartPointer<vtkPolyDataMapper> spControllerMapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    if (m_args.end() == m_args.find("--controllerfile"))
    {
        std::cout << "Controller file is missing. Please specify switch --controllerfile <name_of_.stl_file>" << std::endl;
        ret = 3;
        return ret;
    }

    spControllerFileReader->SetFileName(m_args["--controllerfile"].c_str());
    spControllerFileReader->Update();
    spControllerMapper->SetInputConnection(spControllerFileReader->GetOutputPort());
    spControllerActor->SetMapper(spControllerMapper);
    spControllerActor->SetPosition(spOrbCenterCoordinates->GetValue());
    if (m_args.end() != m_args.find("--controllerscale"))
    {
        scale = stod(m_args.at("--controllerscale"));
    }
    spControllerActor->SetScale(scale);
    if (0 == uIndex)
    {
        spControllerActor->GetProperty()->SetColor(1, 0, 0);
    }
    m_spActorCollection.back()->AddItem(spControllerActor);
    return ret;
}

size_t CBannerSimpleScenario::AddControllerInWindow(vtkSmartPointer<vtkCoordinate> spControllerCenterCoordinates, uint64_t uIndex)
{
    size_t ret = 0;
    if (m_isControllerAnimated)
    {
        m_spActorCollection.push_back(vtkSmartPointer<vtkActorCollection>::New());
        if (0 != (ret = HandleControllerFile(spControllerCenterCoordinates, uIndex)))
        {
            return ret;
        }
        m_spRendererCollection->InitTraversal();
        for (int i = 0; i < m_spRendererCollection->GetNumberOfItems(); ++i)
        {
            const auto&& pRenderer = m_spRendererCollection->GetNextItem();
            for (size_t j = 0; j < m_spActorCollection.size(); ++j)
            {
                m_spActorCollection.back()->InitTraversal();
                vtkActor* const pControllerActor = m_spActorCollection.back()->GetNextActor();
                pRenderer->AddActor(pControllerActor);
                pRenderer->Modified();
            }
        }
    }
    return ret;
}

void CBannerSimpleScenario::GenerateInterpolations(std::string ControllerAnimationFile, vtkSmartPointer<vtkTransformInterpolator> * const spTransformInterpolator)
{
    std::vector<std::vector<double>> AnimationFile;

    // check for animation
    if (m_isControllerAnimated)
    {
        m_controllerAnimationSize.push_back(0);
        controllerAnimationRead(ControllerAnimationFile.c_str(), &m_controllerAnimationSize.back(), &AnimationFile);
        m_controllerAnim.push_back(AnimationFile);

        {
            char controllerAnimFileName[FILE_SIZE_MAX] = { 0 };
            if (0 > snprintf(controllerAnimFileName, sizeof(controllerAnimFileName), "%s/controllerAnim%d.txt", m_szDirectoryName.c_str(), static_cast<int>(m_controllerAnimationSize.size() - 1)))
            {
                cout << "snprintf failed." << "\nLine:" << __LINE__ << "\nFunction:" << __FUNCTION__ << endl;
            }
            m_spControllerAnimFile.push_back(std::make_shared<ofstream>(controllerAnimFileName));
        }
    }

    double current_time = 0.0;
    int count = 0;

    (*spTransformInterpolator)->SetInterpolationTypeToSpline();
    vtkSmartPointer<vtkActor> spActorPosition = vtkSmartPointer<vtkActor>::New();
    while (count < m_controllerAnimationSize.back())
    {
        spActorPosition->SetPosition(AnimationFile[count][1], AnimationFile[count][2], AnimationFile[count][3]);
        (*spTransformInterpolator)->AddTransform(current_time++, spActorPosition);

        spActorPosition->RotateX(AnimationFile[count][4]);
        (*spTransformInterpolator)->AddTransform(current_time++, spActorPosition);

        spActorPosition->RotateY(AnimationFile[count][5]);
        (*spTransformInterpolator)->AddTransform(current_time++, spActorPosition);

        spActorPosition->RotateZ(AnimationFile[count][6]);
        (*spTransformInterpolator)->AddTransform(current_time++, spActorPosition);

        ++count;
    }
}

void CBannerSimpleScenario::StartScenario()
{
    m_spInteractors[0]->Initialize();
    m_spInteractors[0]->CreateRepeatingTimer(m_RenderingTimeInterval);
    m_spInteractors[0]->Start();
}

void CBannerSimpleScenario::CreateTimerWindowCallback()
{
    if (!m_spTimerInput)
    {
        std::vector<double> Times;
        m_spCamerainterp.push_back(vtkSmartPointer<vtkCameraInterpolator>::New());
        CreateCameraInterpolator(m_args["--animation"].c_str(), m_spLFisheyeCapturer->spColorFOV->GetValue()[1], &m_spCamerainterp.back(),&Times);
        m_spTimerInput = std::make_shared<TimerInput>(m_args,Times, &m_frameIndex, m_spActorCollection, m_NumberOfFrames, m_spTransformInterpolatorCollection, &m_isControllerAnimated, m_spCapturers, m_spColorWindow,
            m_spDepthWindow, m_spLFisheyeWindow, m_spRFisheyeWindow, m_spCamerainterp, &(m_spInteractors[0]));
    }
    m_spTimerWindowCallback = vtkSmartPointer<TimerWindowCallback>::New();
    m_spInteractors[0]->AddObserver(vtkCommand::TimerEvent, m_spTimerWindowCallback);
    m_spTimerWindowCallback.GetPointer()->SetInput(m_spTimerInput);
    m_spTimerWindowCallback.GetPointer()->SetWindowsCallbacks(m_spSColorWindowCallback, m_spEColorWindowCallback, m_spELFisheyeWindowCallback, m_spERFisheyeWindowCallback, m_spEDepthWindowCallback);
}

void CBannerSimpleScenario::Execute()
{
    if (m_isAnimationEnabled)
    {
        CreateCamerasCallbacks();
        CreateTimerWindowCallback();
        StartScenario();
    }
    else
    {
        PrintUsage();
    }
}

void CBannerSimpleScenario::AddNewControllerInterpolation(vtkSmartPointer<vtkTransformInterpolator> spNewInterpolation)
{
    m_spTransformInterpolatorCollection.push_back(spNewInterpolation);
}

