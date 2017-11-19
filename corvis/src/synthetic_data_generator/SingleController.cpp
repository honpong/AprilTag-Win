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

void CBannerSimpleScenario::GetControllerActorsCenters(vtkSmartPointer<vtkCoordinate>* const spOrbCenterCoordinates,
    vtkSmartPointer<vtkCoordinate>* const spLEDCenterCoordinates, vtkSmartPointer<vtkCoordinate>* const spBodyCenterCoordinates, int index) const
{
    *spOrbCenterCoordinates = vtkSmartPointer<vtkCoordinate>::New();
    *spLEDCenterCoordinates = vtkSmartPointer<vtkCoordinate>::New();
    *spBodyCenterCoordinates = vtkSmartPointer<vtkCoordinate>::New();
    double actororbx  = 0 == index ? 0.0f     : 0.01f;
    double actororby  = 0 == index ? 0.006f   : 0.008f;
    double actororbz  = 0 == index ? 0.00f    : 0.05f;
    double actorbodyx = 0 == index ? 0.0f     : 0.01f;
    double actorbodyy = 0 == index ? 0.002f   : 0.004f;
    double actorbodyz = 0 == index ? 0.00f    : 0.05f;
    double actorledx  = 0 == index ? 0.0f     : 0.01f;
    double actorledy  = 0 == index ? -0.0015f : 0.00f;
    double actorledz  = 0 == index ? 0.00f    : 0.05f;

    if (0 == index && m_args.find("--orbx") != m_args.end())
    {
        actororbx = stod(m_args.at("--orbx"));
    }
    else if (1 == index && m_args.find("--secondorbx") != m_args.end())
    {
        actororbx = stod(m_args.at("--secondorbx"));
    }

    if (0 == index && m_args.find("--orby") != m_args.end())
    {
        actororby = stod(m_args.at("--orby"));
    }
    else if (1 == index && m_args.find("--secondorby") != m_args.end())
    {
        actororby = stod(m_args.at("--secondorby"));
    }

    if (0 == index && m_args.find("--orbz") != m_args.end())
    {
        actororbz = stod(m_args.at("--orbz"));
    }
    else if (1 == index && m_args.find("--secondorbz") != m_args.end())
    {
        actororbz = stod(m_args.at("--secondorbz"));
    }
    (*spOrbCenterCoordinates)->SetValue(actororbx, actororby, actororbz);

    if (0 == index && m_args.find("--bodyx") != m_args.end())
    {
        actorbodyx = stod(m_args.at("--bodyx"));
    }
    else if (1 == index && m_args.find("--secondbodyx") != m_args.end())
    {
        actorbodyx = stod(m_args.at("--secondbodyx"));
    }

    if (0 == index && m_args.find("--bodyy") != m_args.end())
    {
        actorbodyy = stod(m_args.at("--bodyy"));
    }
    else if (1 == index && m_args.find("--secondbodyy") != m_args.end())
    {
        actorbodyy = stod(m_args.at("--secondbodyy"));
    }

    if (0 == index && m_args.find("--bodyz") != m_args.end())
    {
        actorbodyz = stod(m_args.at("--bodyz"));
    }
    else if (1 == index && m_args.find("--secondbodyz") != m_args.end())
    {
        actorbodyz = stod(m_args.at("--secondbodyz"));
    }
    (*spBodyCenterCoordinates)->SetValue(actorbodyx, actorbodyy, actorbodyz);

    if (0 == index && m_args.find("--ledx") != m_args.end())
    {
        actorledx = stod(m_args.at("--ledx"));
    }
    else if (1 == index && m_args.find("--secondledx") != m_args.end())
    {
        actorledx = stod(m_args.at("--secondledx"));
    }

    if (0 == index && m_args.find("--ledy") != m_args.end())
    {
        actorledy = stod(m_args.at("--ledy"));
    }
    else if (1 == index && m_args.find("--secondledy") != m_args.end())
    {
        actorledy = stod(m_args.at("--secondledy"));
    }

    if (0 == index && m_args.find("--ledz") != m_args.end())
    {
        actorledz = stod(m_args.at("--ledz"));
    }
    else if (1 == index && m_args.find("--secondledz") != m_args.end())
    {
        actorledz = stod(m_args.at("--secondledz"));
    }
    (*spLEDCenterCoordinates)->SetValue(actorledx, actorledy, actorledz);
}

void CBannerSimpleScenario::HandleDirectoryCreation()
{
#ifdef _WIN32
    m_szDirectoryName = std::string("ControllerData") + "\\";
    if (m_args.find("--directory") != m_args.end())
    {
        m_szDirectoryName = m_args["--directory"] + "\\";
    }
    system((std::string("rd /S /Q ") + m_szDirectoryName).c_str());
#else
    m_szDirectoryName = std::string("ControllerData") + "/";
    if (m_args.find("--directory") != m_args.end())
    {
        m_szDirectoryName = m_args["--directory"] + "/";
    }
    system((std::string("rm -rf ") + m_szDirectoryName).c_str());
#endif
    system((std::string("mkdir ") + m_szDirectoryName).c_str());
    system((std::string("mkdir ") + m_szDirectoryName + std::string("color")).c_str());
    system((std::string("mkdir ") + m_szDirectoryName + std::string("depth")).c_str());
    system((std::string("mkdir ") + m_szDirectoryName + std::string("fisheye_0")).c_str());
    system((std::string("mkdir ") + m_szDirectoryName + std::string("fisheye_1")).c_str());
}

unsigned int CBannerSimpleScenario::ParseUserInput(std::map<std::string, std::string> args)
{
    unsigned int uRes = 0;
    m_args = args;

    if (0 != LoadCalibrationFile(m_args))
    {
        std::cout << "Load calibration file failed." << std::endl;
        uRes = 1;
        return uRes;
    }

    m_spMesh = std::make_shared<CMesh>(m_args);
    m_spMesh->Create();

    if (m_args.find("--controlleranimation") != m_args.end())
    {
        m_isControllerAnimated = true;
        if (args.find("--markeroff") != args.end())
        {
            cout << "Marker disabled." << endl;
            m_isMarkerOn.push_back(false);
        }
        else
        {
            m_isMarkerOn.push_back(true);
        }
    }

    HandleDirectoryCreation();

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
        char framesFileName[1024] = { 0 };
        char referenceFileName[1024] = { 0 };
        char animationFileName[1024] = { 0 };
        if (0 > snprintf(framesFileName, sizeof(framesFileName), "%srgbdepthF.txt", m_szDirectoryName.c_str()))
        {
            cout << "snprintf failed." << "\nLine:" << __LINE__ << "\nFunction:" << __FUNCTION__ << endl;
        }
        m_framesFile.open(framesFileName);
        if (0 > snprintf(referenceFileName, sizeof(referenceFileName), "%sOCLRefLogFileF.txt", m_szDirectoryName.c_str()))
        {
            cout << "snprintf failed." << "\nLine:" << __LINE__ << "\nFunction:" << __FUNCTION__ << endl;
        }
        m_referenceFile.open(referenceFileName);
        if (0 > snprintf(animationFileName, sizeof(animationFileName), "%sanim.txt", m_szDirectoryName.c_str()))
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
        uRes = 2;
        return uRes;
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
    return uRes;
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

unsigned int CBannerSimpleScenario::InitializeSetup(std::map<std::string, std::string> args)
{
    unsigned int uRes = 0;
    uRes = ParseUserInput(args);
    if (0 != uRes)
    {
        return uRes;
    }

    CreateCameraWindows();

    if (m_isControllerAnimated)
    {
        m_spActorCollection.push_back(vtkSmartPointer<vtkActorCollection>::New());
        vtkSmartPointer<vtkCoordinate> spOrbCenterCoordinates = vtkSmartPointer<vtkCoordinate>::New();
        vtkSmartPointer<vtkCoordinate> spLEDCenterCoordinates = vtkSmartPointer<vtkCoordinate>::New();
        vtkSmartPointer<vtkCoordinate> spBodyCenterCoordinates = vtkSmartPointer<vtkCoordinate>::New();
        GetControllerActorsCenters(&spOrbCenterCoordinates, &spLEDCenterCoordinates, &spBodyCenterCoordinates, 0);

        m_spOrb.push_back(std::make_shared<CActorSphere>(spOrbCenterCoordinates, 0.02 / 16, 30, 30));
        m_spOrb.back()->Create();

        m_spControllerBody.push_back(std::make_shared<CActorCylinder>(spBodyCenterCoordinates, 0.02 / 16, 100 / 5, 0.08 / 12));
        m_spControllerBody.back()->Create();

        m_spLED.push_back(std::make_shared<CActorSphere>(spLEDCenterCoordinates, 0.01 / 16, 30, 30));
        m_spLED.back()->Create();

        m_spActorCollection.back()->AddItem(m_spOrb.back().get()->m_spActorSphere = vtkSmartPointer<vtkActor>::New());
        m_spActorCollection.back()->AddItem(m_spLED.back().get()->m_spActorSphere = vtkSmartPointer<vtkActor>::New());
        m_spActorCollection.back()->AddItem(m_spControllerBody.back().get()->m_spActorCylinder = vtkSmartPointer<vtkActor>::New());
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

    m_spColorWindow.get()->m_spRenderer->GetActors()->InitTraversal();
    std::cout << "Number of items:" << (int)m_spColorWindow.get()->m_spRenderer->GetActors()->GetNumberOfItems() << endl;
    for (unsigned int i = 0; i < m_spColorWindow.get()->m_spRenderer->GetActors()->GetNumberOfItems(); ++i)
    {
        //vtkActor* const pA = m_spColorWindow.get()->m_spRenderer->GetActors()->GetNextActor();
    }


    if (m_isControllerAnimated)
    {
        m_spRendererCollection->InitTraversal();
        for (int i = 0; i < m_spRendererCollection->GetNumberOfItems(); ++i)
        {
            const auto&& pRenderer = m_spRendererCollection->GetNextItem();

            for (uint64_t i = 0; i < static_cast<uint64_t>(m_spActorCollection.size()); ++i)
            {
                vtkSmartPointer<vtkPolyDataMapper> spMapperSphere = vtkSmartPointer<vtkPolyDataMapper>::New();
                vtkSmartPointer<vtkPolyDataMapper> spMapperLED = vtkSmartPointer<vtkPolyDataMapper>::New();
                spMapperSphere->SetInputConnection(m_spOrb[i].get()->m_spPolygonSphereSource->GetOutputPort());
                spMapperLED->SetInputConnection(m_spLED[i].get()->m_spPolygonSphereSource->GetOutputPort());
                m_spActorCollection[i]->InitTraversal();

                vtkActor* const pActorOrb = m_spActorCollection[i]->GetNextActor();
                pActorOrb->SetMapper(spMapperSphere);
                pActorOrb->GetProperty()->SetColor(1, 1, 1);
                pActorOrb->GetProperty()->SetColor(255, 255, 255);
                pActorOrb->GetProperty()->LightingOff();
                pRenderer->AddActor(pActorOrb);
                pRenderer->Modified();

                vtkActor* const pActorLED = m_spActorCollection[i]->GetNextActor();
                pActorLED->SetMapper(spMapperLED);
                pActorLED->GetProperty()->SetColor(1, 0, 0);
                pActorLED->GetProperty()->LightingOff();
                if (m_isMarkerOn[i])
                {
                    pRenderer->AddActor(pActorLED);
                    pRenderer->Modified();
                }

                vtkActor* const pActorCylinder = m_spActorCollection[i]->GetNextActor();
                vtkSmartPointer<vtkPolyDataMapper> spMapperCylinder = vtkSmartPointer<vtkPolyDataMapper>::New();
                spMapperCylinder->SetInputConnection(m_spControllerBody[i].get()->m_spCylinderSource->GetOutputPort());
                pActorCylinder->SetMapper(spMapperCylinder);
                pActorCylinder->GetProperty()->SetColor(0.0, 1.0, 0.0);
                pRenderer->AddActor(pActorCylinder);
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

    return uRes;
}

void CBannerSimpleScenario::CreateCamerasCallbacks()
{
    std::shared_ptr<EndCallback::CallbackInput> spEndInput = std::make_shared<EndCallback::CallbackInput>(m_spCapturers, m_szDirectoryName, &m_framesFile, &m_referenceFile, &m_animationFile, m_spSColorWindowCallback.GetPointer(), &m_frameIndex, m_NumberOfFrames, &m_isFisheyeStereoRecordingEnabled, &m_isRecordingEnabled,
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
    for (auto & i : m_spControllerBody)
    {
        spIMUOnController.push_back(i.get()->m_spActorCylinder);
    }
    std::shared_ptr<StartCallback::CallbackInput> spStartInput = std::make_shared<StartCallback::CallbackInput>(m_spControllerAnimFile, &m_frameIndex, &m_isControllerAnimated, spIMUOnController,
        &(m_spColorWindow.get()->m_spRenderWindow), &(m_spDepthWindow.get()->m_spRenderWindow), &(m_spLFisheyeWindow.get()->m_spRenderWindow), &(m_spRFisheyeWindow.get()->m_spRenderWindow));
    if (m_spSColorWindowCallback)
    {
        m_spSColorWindowCallback.GetPointer()->InitializeInputs(spStartInput);
    }
}

void CBannerSimpleScenario::AddControllerInWindow(vtkSmartPointer<vtkCoordinate> spOrbCenterCoordinates, vtkSmartPointer<vtkCoordinate> spLEDCenterCoordinates, vtkSmartPointer<vtkCoordinate> spBodyCenterCoordinates, bool isMarkerOn)
{
    if (m_isControllerAnimated)
    {
        m_spActorCollection.push_back(vtkSmartPointer<vtkActorCollection>::New());
        m_spOrb.push_back(std::make_shared<CActorSphere>(spOrbCenterCoordinates, 0.02 / 16, 30, 30));
        m_spOrb.back()->Create();

        m_spControllerBody.push_back(std::make_shared<CActorCylinder>(spBodyCenterCoordinates, 0.02 / 16, 100 / 5, 0.08 / 12));
        m_spControllerBody.back()->Create();

        m_spLED.push_back(std::make_shared<CActorSphere>(spLEDCenterCoordinates, 0.01 / 16, 30, 30));
        m_spLED.back()->Create();

        m_spActorCollection.back()->AddItem(m_spOrb.back().get()->m_spActorSphere = vtkSmartPointer<vtkActor>::New());
        m_spActorCollection.back()->AddItem(m_spLED.back().get()->m_spActorSphere = vtkSmartPointer<vtkActor>::New());
        m_spActorCollection.back()->AddItem(m_spControllerBody.back().get()->m_spActorCylinder = vtkSmartPointer<vtkActor>::New());
        m_isMarkerOn.push_back(isMarkerOn);

        m_spRendererCollection->InitTraversal();
        for (int i = 0; i < m_spRendererCollection->GetNumberOfItems(); ++i)
        {
            const auto&& pRenderer = m_spRendererCollection->GetNextItem();

            for (uint64_t j = 0; j < static_cast<uint64_t>(m_spActorCollection.size()); ++j)
            {
                vtkSmartPointer<vtkPolyDataMapper> spMapperSphere = vtkSmartPointer<vtkPolyDataMapper>::New();
                vtkSmartPointer<vtkPolyDataMapper> spMapperLED = vtkSmartPointer<vtkPolyDataMapper>::New();
                spMapperSphere->SetInputConnection(m_spOrb.back().get()->m_spPolygonSphereSource->GetOutputPort());
                spMapperLED->SetInputConnection(m_spLED.back().get()->m_spPolygonSphereSource->GetOutputPort());
                m_spActorCollection.back()->InitTraversal();
                vtkActor* const pActorOrb = m_spActorCollection.back()->GetNextActor();
                pActorOrb->SetMapper(spMapperSphere);
                pActorOrb->GetProperty()->SetColor(1, 1, 1);
                pActorOrb->GetProperty()->SetColor(255, 255, 255);
                pActorOrb->GetProperty()->LightingOff();
                pRenderer->AddActor(pActorOrb);
                pRenderer->Modified();

                vtkActor* const pActorLED = m_spActorCollection.back()->GetNextActor();
                pActorLED->SetMapper(spMapperLED);
                pActorLED->GetProperty()->SetColor(1, 0, 0);
                pActorLED->GetProperty()->LightingOff();
                if (m_isMarkerOn.back())
                {
                    pRenderer->AddActor(pActorLED);
                    pRenderer->Modified();
                }

                vtkActor* const pActorCylinder = m_spActorCollection.back()->GetNextActor();
                vtkSmartPointer<vtkPolyDataMapper> spMapperCylinder = vtkSmartPointer<vtkPolyDataMapper>::New();
                spMapperCylinder->SetInputConnection(m_spControllerBody.back().get()->m_spCylinderSource->GetOutputPort());
                pActorCylinder->SetMapper(spMapperCylinder);
                pActorCylinder->GetProperty()->SetColor(0.0, 1.0, 0.0);
                pRenderer->AddActor(pActorCylinder);
                pRenderer->Modified();
            }
        }
    }
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
            char controllerAnimFileName[1024] = { 0 };
            if (0 > snprintf(controllerAnimFileName, sizeof(controllerAnimFileName), "%scontrollerAnim%d.txt", m_szDirectoryName.c_str(), static_cast<int>(m_controllerAnimationSize.size() - 1)))
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
        CreateCameraInterpolator(m_args["--animation"].c_str(), m_spColorCapturer->spColorFOV->GetValue()[1], &m_spCamerainterp.back(),&Times);
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

