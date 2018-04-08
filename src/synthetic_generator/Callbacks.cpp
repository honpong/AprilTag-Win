#include <thread>
#include <mutex>
#include "Utils.h"
#include <new>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include "Callbacks.h"
#include "ScenesObjects.h"

StartCallback::CallbackInput::CallbackInput(std::vector<std::shared_ptr<std::ofstream>>& spControllerAnimFile, const int* const pframeIndex, const bool* const pIsControllerAnimated, std::vector<vtkSmartPointer<vtkActor>> spActorSpheres,
    vtkSmartPointer<vtkRenderWindow>* const spColorRenderwindow, vtkSmartPointer<vtkRenderWindow>* const spDepthRenderwindow, vtkSmartPointer<vtkRenderWindow>* const spLFisheyeRenderwindow, vtkSmartPointer<vtkRenderWindow>* const spRFisheyeRenderwindow)
    : m_spControllerAnimFile(spControllerAnimFile), m_pframeIndex(pframeIndex), m_pIsControlleranimated(pIsControllerAnimated), m_spActorSpheres(spActorSpheres),
    m_spColorRenderwindow(spColorRenderwindow), m_spDepthRenderwindow(spDepthRenderwindow), m_spLFisheyeRenderwindow(spLFisheyeRenderwindow), m_spRFisheyeRenderwindow(spRFisheyeRenderwindow)
{
}

ColorWindowStartCallback*  ColorWindowStartCallback::New()
{
    return new ColorWindowStartCallback();
}

void ColorWindowStartCallback::InitializeInputs(std::shared_ptr<CallbackInput> spInput)
{
    m_spInput = spInput;
    lastProcessedFrame = -1;
}

double ColorWindowStartCallback::GetLastProcessedFrame() const
{
    return lastProcessedFrame;
}

void StartCallback::Execute(vtkObject*, unsigned long, void*)
{
    if (m_spInput && *(m_spInput->m_pframeIndex) > lastProcessedFrame && 0 <= *(m_spInput->m_pframeIndex) && *(m_spInput->m_pIsControlleranimated))
    {
        if(m_mutex.try_lock())
        {
            for (uint64_t i = 0; i < static_cast<uint64_t>((m_spInput->m_spActorSpheres).size()); ++i)
            {
                vtkSmartPointer<vtkMatrix4x4> spM = vtkSmartPointer<vtkMatrix4x4>::New();
                m_spInput->m_spActorSpheres[i]->GetMatrix(spM);
                *(m_spInput->m_spControllerAnimFile)[i] << *(m_spInput->m_pframeIndex);
                (*(m_spInput->m_spControllerAnimFile)[i]).setf(ios::right, ios::adjustfield);
                *(m_spInput->m_spControllerAnimFile)[i] << std::fixed << setw(12) << std::setprecision(8) << spM->GetElement(0, 0) << " " << spM->GetElement(0, 1) << " " << spM->GetElement(0, 2) << " " << spM->GetElement(0, 3);
                *(m_spInput->m_spControllerAnimFile)[i] << std::fixed << setw(12) << std::setprecision(8) << spM->GetElement(1, 0) << " " << spM->GetElement(1, 1) << " " << spM->GetElement(1, 2) << " " << spM->GetElement(1, 3);
                *(m_spInput->m_spControllerAnimFile)[i] << std::fixed << setw(12) << std::setprecision(8) << spM->GetElement(2, 0) << " " << spM->GetElement(2, 1) << " " << spM->GetElement(2, 2) << " " << spM->GetElement(2, 3);
                *(m_spInput->m_spControllerAnimFile)[i] << "\n";
            }
            m_mutex.unlock();
        }
    }
    lastProcessedFrame = m_spInput ? *(m_spInput->m_pframeIndex) : ++lastProcessedFrame;
}

EndCallback::CallbackInput::CallbackInput(std::vector<std::shared_ptr<CFakeImgCapturer>> spCap, std::ofstream* const pFramesFile, std::ofstream* const pReferenceFile, std::ofstream* const pAnimationFile, vtkCommand* const pStartCallback, int* const pframeIndex, int MaximumFrames, const bool* const pIsFisheyeStereoRecordingEnabled, const bool* const pIsRecordingEnabled, vtkSmartPointer<vtkRenderer>* const spColorRenderer, vtkSmartPointer<vtkRenderer>* const spDepthRenderer,
    vtkSmartPointer<vtkRenderWindow>* const spColorRenderwindow, vtkSmartPointer<vtkRenderWindow>* const spDepthRenderwindow, vtkSmartPointer<vtkRenderWindow>* const spLFisheyeRenderwindow, vtkSmartPointer<vtkRenderWindow>* const spRFisheyeRenderwindow) :
    m_spCapturers(spCap), m_pFramesFile(pFramesFile), m_pReferenceFile(pReferenceFile), m_pAnimationFile(pAnimationFile), m_pStartCallback(pStartCallback), m_pframeIndex(pframeIndex), m_MaximumFrames(MaximumFrames), m_pIsFisheyeStereoRecordingEnabled(pIsFisheyeStereoRecordingEnabled), m_pIsRecordingEnabled(pIsRecordingEnabled), m_spColorRenderer(spColorRenderer), m_spDepthRenderer(spDepthRenderer),
    m_spColorRenderwindow(spColorRenderwindow), m_spDepthRenderwindow(spDepthRenderwindow), m_spLFisheyeRenderwindow(spLFisheyeRenderwindow), m_spRFisheyeRenderwindow(spRFisheyeRenderwindow)
{
}

double EndCallback::GetLastProcessedFrame() const
{
    return lastProcessedFrame;
}

void EndCallback::InitializeInputs(std::shared_ptr<CallbackInput> spInput)
{
    m_spInput = spInput;
    m_max = 100000000000000;
    lastProcessedFrame = -1;
    // matrices we need
    m_spTi = vtkSmartPointer<vtkMatrix4x4>::New();
    m_spP = vtkSmartPointer<vtkMatrix4x4>::New();
    m_spInvT0 = vtkSmartPointer<vtkMatrix4x4>::New();
    m_spTmpP = vtkSmartPointer<vtkMatrix4x4>::New();
    m_spCoordR = vtkSmartPointer<vtkMatrix4x4>::New();
    m_spCoordR->SetElement(0, 0, 1.0);
    m_spCoordR->SetElement(1, 1, -1.0);
    m_spCoordR->SetElement(2, 2, -1.0);
    m_spCoordR->SetElement(3, 3, 1.0);
    m_spTrans2 = vtkSmartPointer<vtkMatrix4x4>::New();
    m_spTrans2->SetElement(0, 0, 1.0);
    m_spTrans2->SetElement(1, 1, 1.0);
    m_spTrans2->SetElement(2, 2, 1.0);
    m_spTrans2->SetElement(3, 3, 1.0);
}

ColorWindowEndCallback* ColorWindowEndCallback::New()
{
    return new ColorWindowEndCallback();
}

void ColorWindowEndCallback::Execute(vtkObject*, unsigned long, void*)
{
    vtkSmartPointer<vtkWindowToImageFilter> spWindowToImage;
    vtkImageData* pImgdata = nullptr;

    if (m_spInput && *(m_spInput->m_pframeIndex) > lastProcessedFrame && 0 <= *(m_spInput->m_pframeIndex) &&  *(m_spInput->m_pframeIndex) < m_spInput->m_MaximumFrames)
    {
        if (m_mutex.try_lock())
        {
            if (!m_spColorCapturer)
            {
                std::shared_ptr<CFakeImgCapturer> spFisheye;
                for (const auto& i : m_spInput->m_spCapturers)
                {
                    if (rc_FORMAT_DEPTH16 == i->ImageFormat)
                    {
                        m_spColorCapturer = i;
                        break;
                    }
                    else if ((rc_FORMAT_RGBA8 == i->ImageFormat) && (0 == i->CameraIndex))
                    {
                        m_spColorCapturer = i;
                    }
                }
            }
            if (m_spColorCapturer)
            {
                if (*(m_spInput->m_pIsRecordingEnabled))
                {
                    // color
                    (*(m_spInput->m_spColorRenderwindow))->RemoveObserver(this);
                    (*(m_spInput->m_spColorRenderwindow))->RemoveObserver(m_spInput->m_pStartCallback);

                    spWindowToImage = vtkSmartPointer<vtkWindowToImageFilter>::New();
                    spWindowToImage->SetInput((*(m_spInput->m_spColorRenderwindow)));
                    spWindowToImage->SetInputBufferTypeToRGBA();
                    spWindowToImage->Update();
                    pImgdata = spWindowToImage->GetOutput();
                    const unsigned char* pColorPix = reinterpret_cast<unsigned char*>(pImgdata->GetScalarPointer());
                    int cfd_size = m_spColorCapturer->ColorResolution.width * m_spColorCapturer->ColorResolution.height * 4;
                    std::unique_ptr<unsigned char[]> cfd = std::make_unique<unsigned char[]>(cfd_size);
                    for (int i = 0; i < m_spColorCapturer->ColorResolution.width * m_spColorCapturer->ColorResolution.height * 4; i += 4)
                    {
                        cfd[i] = pColorPix[i];
                        cfd[i + 1] = pColorPix[i + 1];
                        cfd[i + 2] = pColorPix[i + 2];
                        cfd[i + 3] = pColorPix[i + 3];
                    }
                    (*(m_spInput->m_spColorRenderwindow))->AddObserver(vtkCommand::EndEvent, this);
                    (*(m_spInput->m_spColorRenderwindow))->AddObserver(vtkCommand::StartEvent, m_spInput->m_pStartCallback);

                    // write data
                    Record(cfd.get(), cfd_size, *(m_spInput->m_pframeIndex), m_spColorCapturer, m_spInput->m_pFramesFile);

                    // Write the pose information to file streams
                    vtkMatrix4x4::Multiply4x4(m_spCoordR, (*m_spInput->m_spColorRenderer)->GetActiveCamera()->GetModelViewTransformMatrix(), m_spTi);
                    m_spTi->Invert();
                    if (*(m_spInput->m_pframeIndex) == 0)
                    {
                        m_spInvT0->DeepCopy(m_spTi);
                        m_spInvT0->Invert();
                    }

                    vtkMatrix4x4::Multiply4x4(m_spInvT0, m_spTi, m_spTmpP); // pose relative to first camera
                    vtkMatrix4x4::Multiply4x4(m_spTrans2, m_spTmpP, m_spP); // pose relative to shifted (2,2,0) first camera

                    char relativeFilePath[FILE_SIZE_MAX] = { 0 };
                    if (0 > snprintf(relativeFilePath, sizeof(relativeFilePath), LFISHEYE_RELATIVE_PATH, *(m_spInput->m_pframeIndex)))
                    {
                        cout << "snprintf failed." << "\nLine:" << __LINE__ << "\nFunction:" << __FUNCTION__ << endl;
                    }

                    *m_spInput->m_pReferenceFile << relativeFilePath;
                    for (int i = 0; i < 12; ++i)
                    {
                        (*m_spInput->m_pReferenceFile).setf(ios::right, ios::adjustfield);
                        *m_spInput->m_pReferenceFile << " " << std::fixed << setw(12) << std::setprecision(8) << m_spP->GetElement(i / 4, i % 4);//ptr[i];
                    }
                    *m_spInput->m_pReferenceFile << "\n";
                }

                // print camera information for animation
                double params[12] = { 0 };
                vtkCamera* cam1 = (*m_spInput->m_spColorRenderer)->GetActiveCamera();
                cam1->GetPosition(params[0], params[1], params[2]);
                cam1->GetFocalPoint(params[3], params[4], params[5]);
                cam1->GetViewUp(params[6], params[7], params[8]);
                cam1->GetDirectionOfProjection(params[9], params[10], params[11]);

                *m_spInput->m_pAnimationFile << *(m_spInput->m_pframeIndex);
                for (int i = 0; i < 9; ++i)
                {
                    (*m_spInput->m_pAnimationFile).setf(ios::right, ios::adjustfield);
                    *m_spInput->m_pAnimationFile << " " << std::fixed << setw(12) << std::setprecision(8) << params[i];
                }
                *m_spInput->m_pAnimationFile << "\n";
                if (!(*(m_spInput->m_pIsRecordingEnabled)))
                {
                    // Write the pose information to file streams
                    vtkMatrix4x4::Multiply4x4(m_spCoordR, (*m_spInput->m_spColorRenderer)->GetActiveCamera()->GetModelViewTransformMatrix(), m_spTi);
                    m_spTi->Invert();
                    if (0 == *(m_spInput->m_pframeIndex))
                    {
                        m_spInvT0->DeepCopy(m_spTi);
                        m_spInvT0->Invert();
                    }

                    vtkMatrix4x4::Multiply4x4(m_spInvT0, m_spTi, m_spTmpP); // pose relative to first camera
                    vtkMatrix4x4::Multiply4x4(m_spTrans2, m_spTmpP, m_spP); // pose relative to shifted (2,2,0) first camera

                    for (int i = 0; i < 12; ++i)
                    {
                        std::cout.setf(ios::right, ios::adjustfield);
                        std::cout << " " << std::fixed << setw(12) << std::setprecision(8) << m_spP->GetElement(i / 4, i % 4);//ptr[i];
                    }
                    std::cout << "\n";
                }
            }
            lastProcessedFrame = m_spInput ? *(m_spInput->m_pframeIndex) : ++lastProcessedFrame;
            m_mutex.unlock();
        }
    }
    else if (*(m_spInput->m_pframeIndex) < 0)
    {
        lastProcessedFrame = m_spInput ? *(m_spInput->m_pframeIndex) : ++lastProcessedFrame;
    }
}

void ColorWindowEndCallback::Record(unsigned char* const cfd, int cfd_size, int currentFrameIndex, std::shared_ptr<CFakeImgCapturer> spcap, std::ofstream* const pFramesFile)
{
    vtkSmartPointer<vtkImageImport> spImporter = vtkSmartPointer<vtkImageImport>::New();
    vtkSmartPointer<vtkPNGWriter> spWriter = vtkSmartPointer<vtkPNGWriter>::New();
    char fileName[FILE_SIZE_MAX] = { 0 };
    if (0 > snprintf(fileName, sizeof(fileName), spcap->GetDirectoryName().c_str(), currentFrameIndex))
    {
        cout << "snprintf failed." << "\nLine:" << __LINE__ << "\nFunction:" << __FUNCTION__ << endl;
    }

    spImporter->CopyImportVoidPointer(cfd, static_cast<int>(cfd_size));
    spImporter->SetDataScalarTypeToUnsignedChar();
    spImporter->SetNumberOfScalarComponents(4);
    spImporter->SetWholeExtent(0, spcap->ColorResolution.width - 1, 0, spcap->ColorResolution.height - 1, 0, 0);
    spImporter->SetDataExtentToWholeExtent();
    spImporter->Update();
    spWriter->SetFileName(fileName);
    spWriter->SetInputConnection(spImporter->GetOutputPort());
    spWriter->Write();
    if(0 > snprintf(fileName, sizeof(fileName), COLOR_RELATIVE_PATH, currentFrameIndex))
    {
        cout << "snprintf failed." << "\nLine:" << __LINE__ << "\nFunction:" << __FUNCTION__ << endl;
    }

    *pFramesFile << currentFrameIndex << " " << fileName;
}

void DepthWindowEndCallback::Record(unsigned short* const dfd, int dfd_size, int currentFrameIndex, std::shared_ptr<CFakeImgCapturer> spcap, std::ofstream* const pFramesFile)
{
    vtkSmartPointer<vtkImageImport> spImporter = vtkSmartPointer<vtkImageImport>::New();
    vtkSmartPointer<vtkPNGWriter> spWriter = vtkSmartPointer<vtkPNGWriter>::New();
    char fileName[FILE_SIZE_MAX] = { 0 };
    if(0 > snprintf(fileName, sizeof(fileName), spcap->GetDirectoryName().c_str(), currentFrameIndex))
    {
        cout << "snprintf failed." << "\nLine:" << __LINE__ << "\nFunction:" << __FUNCTION__ << endl;
    }
    spImporter->CopyImportVoidPointer(dfd, static_cast<int>(dfd_size));
    spImporter->SetDataScalarTypeToUnsignedShort();
    spImporter->SetNumberOfScalarComponents(1);
    spImporter->SetWholeExtent(0, spcap->DepthResolution.width - 1, 0, spcap->DepthResolution.height - 1, 0, 0);
    spImporter->SetDataExtentToWholeExtent();
    spImporter->Update();
    spWriter->SetFileName(fileName);
    spWriter->SetInputConnection(spImporter->GetOutputPort());
    spWriter->Write();
    if(0 > snprintf(fileName, sizeof(fileName), DEPTH_RELATIVE_PATH, currentFrameIndex))
    {
        cout << "snprintf failed." << "\nLine:" << __LINE__ << "\nFunction:" << __FUNCTION__ << endl;
    }
    *pFramesFile << currentFrameIndex << " " << fileName;
}

DepthWindowEndCallback* DepthWindowEndCallback::New()
{
    return new DepthWindowEndCallback();
}

TimerWindowCallback* TimerWindowCallback::New()
{
    return new TimerWindowCallback();
}

void TimerWindowCallback::SetInput(const std::shared_ptr<TimerInput>& spInput)
{
    m_spInputTimer = spInput;
}

TimerInput::TimerInput(std::map<std::string, std::string> args, std::vector<double> times, int * const pFrameIndex, std::vector<vtkSmartPointer<vtkActorCollection>> spActorCollection, unsigned int NumberFrames, std::vector<vtkSmartPointer<vtkTransformInterpolator>>  spTransformInterpolator, bool* isControllerAnimated, std::vector<std::shared_ptr<CFakeImgCapturer>> spCap, std::shared_ptr<CSimulatedWindow> pColorWindow, std::shared_ptr<CSimulatedWindow> pDepthWindow, std::shared_ptr<CSimulatedWindow> pLFisheyeWindow, std::shared_ptr<CSimulatedWindow> pRFisheyeWindow, std::vector<vtkSmartPointer<vtkCameraInterpolator>> spCamerainterp, vtkSmartPointer<vtkRenderWindowInteractor> * pInteractor) :
    m_pColorWindowTimer(pColorWindow),
    m_pDepthWindowTimer(pDepthWindow),
    m_pLFisheyeWindowTimer(pLFisheyeWindow),
    m_pRFisheyeWindowTimer(pRFisheyeWindow),
    m_spCamerainterpTimer(spCamerainterp),
    m_spCapturersTimer(spCap),
    m_pIsControllerAnimatedTimer(isControllerAnimated),
    m_spTransformInterpolatorCollection(spTransformInterpolator),
    m_TotalNumberFrames(NumberFrames),
    m_spActorCollection(spActorCollection),
    m_spInteractorTimer(pInteractor),
    m_pFrameIndex(pFrameIndex),
    m_args(args),
    m_times(times)
{
    m_Max = VTK_DOUBLE_MIN;
    m_Min = VTK_DOUBLE_MAX;

    for (auto & pInterpolator : m_spTransformInterpolatorCollection)
    {
        if (pInterpolator->GetMaximumT() > m_Max)
        {
            m_Max = pInterpolator->GetMaximumT();
        }

        if (pInterpolator->GetMinimumT() < m_Min)
        {
            m_Min = pInterpolator->GetMinimumT();
        }
    }
}

void TimerWindowCallback::SetWindowsCallbacks(vtkSmartPointer<ColorWindowStartCallback> spSColorWindowCallback, vtkSmartPointer<ColorWindowEndCallback> spEColorWindowCallback,
    vtkSmartPointer<LFisheyeWindowEndCallback> spELFisheyeWindowCallback, vtkSmartPointer<RFisheyeWindowEndCallback> spERFisheyeWindowCallback,
    vtkSmartPointer<DepthWindowEndCallback> spEDepthWindowCallback)
{
    m_spSColorWindowCallback = spSColorWindowCallback;
    m_spEColorWindowCallback = spEColorWindowCallback;
    m_spELFisheyeWindowCallback = spELFisheyeWindowCallback;
    m_spERFisheyeWindowCallback = spERFisheyeWindowCallback;
    m_spEDepthWindowCallback = spEDepthWindowCallback;
}

void TimerWindowCallback::Execute(vtkObject*, unsigned long eventId, void*)
{
    if (vtkCommand::TimerEvent == eventId)
    {
        {
            std::unique_lock<std::mutex> lock(m_mutex,std::try_to_lock);
            if (!lock.owns_lock())
                return;

            ++(*m_spInputTimer->m_pFrameIndex);

            CSimulatedWindow* const & pColorWindow = m_spInputTimer->m_pColorWindowTimer.get();
            CSimulatedWindow* const & pLFisheyeWindow = m_spInputTimer->m_pLFisheyeWindowTimer.get();
            CSimulatedWindow* const & pRFisheyeWindow = m_spInputTimer->m_pRFisheyeWindowTimer.get();
            CSimulatedWindow* const & pDepthWindow = m_spInputTimer->m_pDepthWindowTimer.get();

            if ((!m_spColorCapturer) && (!m_spDepthCapturer) && (!m_spLFisheyeCapturer) && (!m_spRFisheyeCapturer))
            {
                for (const auto& i : m_spInputTimer->m_spCapturersTimer)
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

            if (*m_spInputTimer->m_pFrameIndex < m_spInputTimer->m_TotalNumberFrames)
            {
                for (const auto& i : m_spInputTimer->m_spCamerainterpTimer)
                {
                    if (m_spInputTimer->m_args.end() == m_spInputTimer->m_args.find("--nointerpolation"))
                    {
                        i->InterpolateCamera(i->GetMinimumT() + (*m_spInputTimer->m_pFrameIndex * (i->GetMaximumT() - i->GetMinimumT()) / m_spInputTimer->m_TotalNumberFrames), pColorWindow->m_spRenderer->GetActiveCamera());
                    }
                    else if(m_spInputTimer->m_args.end() != m_spInputTimer->m_args.find("--nointerpolation") && *m_spInputTimer->m_pFrameIndex >= 0 && *m_spInputTimer->m_pFrameIndex < static_cast<int>(m_spInputTimer->m_times.size()))
                    {
                        i->InterpolateCamera((m_spInputTimer->m_times)[*m_spInputTimer->m_pFrameIndex], pColorWindow->m_spRenderer->GetActiveCamera());
                    }
                    else if (m_spInputTimer->m_args.end() != m_spInputTimer->m_args.find("--nointerpolation") && *m_spInputTimer->m_pFrameIndex >= static_cast<int>(m_spInputTimer->m_times.size()))
                    {
                        std::cout << "Exiting application." << std::endl;
                        (*(m_spInputTimer->m_spInteractorTimer))->TerminateApp();
                        return;
                    }
                }

                // get color wTc transform
                vtkSmartPointer<vtkMatrix4x4> spWTC = vtkSmartPointer<vtkMatrix4x4>::New();
                spWTC->DeepCopy(pColorWindow->m_spRenderer->GetActiveCamera()->GetModelViewTransformMatrix());
                spWTC->Invert();

                //OpenGL to OpenCV camera coordinate system. Compared to OpenCV, OpenGL coordinate system is flipped by 180 degree around x axis.
                vtkSmartPointer<vtkMatrix3x3> spCameraPosition = vtkSmartPointer<vtkMatrix3x3>::New();
                spCameraPosition->SetElement(0, 0, static_cast<float>(spWTC->GetElement(0, 0)));
                spCameraPosition->SetElement(0, 1, static_cast<float>(-spWTC->GetElement(0, 1)));
                spCameraPosition->SetElement(0, 2, static_cast<float>(-spWTC->GetElement(0, 2)));

                spCameraPosition->SetElement(1, 0, static_cast<float>(spWTC->GetElement(1, 0)));
                spCameraPosition->SetElement(1, 1, static_cast<float>(-spWTC->GetElement(1, 1)));
                spCameraPosition->SetElement(1, 2, static_cast<float>(-spWTC->GetElement(1, 2)));

                spCameraPosition->SetElement(2, 0, static_cast<float>(spWTC->GetElement(2, 0)));
                spCameraPosition->SetElement(2, 1, static_cast<float>(-spWTC->GetElement(2, 1)));
                spCameraPosition->SetElement(2, 2, static_cast<float>(-spWTC->GetElement(2, 2)));

                // get color camera settings
                double pos1[3] = { 0 };
                pColorWindow->m_spRenderer->GetActiveCamera()->GetPosition(pos1);
                double up1[3] = { 0 };
                pColorWindow->m_spRenderer->GetActiveCamera()->GetViewUp(up1);
                double dp1[3] = { 0 };
                pColorWindow->m_spRenderer->GetActiveCamera()->GetDirectionOfProjection(dp1);

                //Aligning the color camera wrt to fisheye
                double pos3[3] = { pos1[0],pos1[1],pos1[2] };
                double up3[3] = { up1[0],up1[1],up1[2] };
                double dp3[3] = { dp1[0],dp1[1],dp1[2] };
                if (pLFisheyeWindow)
                {
                    pLFisheyeWindow->m_spRenderer->GetActiveCamera()->SetPosition(pos3);
                    double cam3FocalPointPosition[3] = { pos3[0] + dp3[0], pos3[1] + dp3[1], pos3[2] + dp3[2] };
                    pLFisheyeWindow->m_spRenderer->GetActiveCamera()->SetFocalPoint(cam3FocalPointPosition);
                    pLFisheyeWindow->m_spRenderer->GetActiveCamera()->SetViewUp(up3);
                }
                // set new position for stereo fisheye
                if (m_spRFisheyeCapturer)
                {
                    double pos4[3] = { 0 };
                    const double secondFisheyeTranslationVector[] = { m_spRFisheyeCapturer->SecondFisheye.TranslationParameters[0], m_spRFisheyeCapturer->SecondFisheye.TranslationParameters[1], m_spRFisheyeCapturer->SecondFisheye.TranslationParameters[2] };
                    MapVectorFromWorldToCamera(secondFisheyeTranslationVector, pos4, spCameraPosition);
                    vtkMath::Add(pos3, pos4, pos4);
                    pRFisheyeWindow->m_spRenderer->GetActiveCamera()->SetPosition(pos4);

                    // set new direction for stereo fisheye
                    spCameraPosition->Transpose();
                    MapVectorFromWorldToCamera(dp3, dp3, spCameraPosition);

                    double dp4[3] = { 0 };
                    MapVectorFromWorldToCamera(dp3, dp4, m_spRFisheyeCapturer->SecondFisheye.spOrientation);
                    MapVectorFromCameraToWorld(dp4, dp4, spCameraPosition);

                    float d4 = sqrt(dp4[0] * dp4[0] + dp4[1] * dp4[1] + dp4[2] * dp4[2]);
                    dp4[0] *= 1.0f / d4;
                    dp4[1] *= 1.0f / d4;
                    dp4[2] *= 1.0f / d4;
                    double cam4FocalPointPosition[3] = { pos4[0] + dp4[0], pos4[1] + dp4[1], pos4[2] + dp4[2] };
                    pRFisheyeWindow->m_spRenderer->GetActiveCamera()->SetFocalPoint(cam4FocalPointPosition);

                    // set the up direction for the stereo fisheye
                    MapVectorFromWorldToCamera(up3, up3, spCameraPosition);

                    double up4[3] = { 0 };
                    MapVectorFromWorldToCamera(up3, up4, m_spRFisheyeCapturer->SecondFisheye.spOrientation);
                    MapVectorFromCameraToWorld(up4, up4, spCameraPosition);

                    d4 = sqrt(up4[0] * up4[0] + up4[1] * up4[1] + up4[2] * up4[2]);
                    up4[0] *= 1.0f / d4;
                    up4[1] *= 1.0f / d4;
                    up4[2] *= 1.0f / d4;
                    pRFisheyeWindow->m_spRenderer->GetActiveCamera()->SetViewUp(up4);
                }

                if (m_spDepthCapturer)
                {
                    // set new position for depth camera
                    spCameraPosition->Transpose();
                    double pos2[3] = { 0 };
                    const double firstFisheyeTranslationVector[] = { m_spDepthCapturer->FirstFisheye.TranslationParameters[0], m_spDepthCapturer->FirstFisheye.TranslationParameters[1],m_spDepthCapturer->FirstFisheye.TranslationParameters[2] };
                    MapVectorFromWorldToCamera(firstFisheyeTranslationVector, pos2, spCameraPosition);
                    vtkMath::Add(pos1, pos2, pos2);
                    pDepthWindow->m_spRenderer->GetActiveCamera()->SetPosition(pos2);

                    // set new direction for the depth camera
                    double dp2[3] = { 0 };
                    spCameraPosition->Transpose();
                    MapVectorFromWorldToCamera(dp1, dp1, spCameraPosition);
                    MapVectorFromWorldToCamera(dp1, dp2, m_spDepthCapturer->FirstFisheye.spOrientation);
                    MapVectorFromCameraToWorld(dp2, dp2, spCameraPosition);

                    float d2 = sqrt(dp2[0] * dp2[0] + dp2[1] * dp2[1] + dp2[2] * dp2[2]);
                    dp2[0] *= 1.0f / d2;
                    dp2[1] *= 1.0f / d2;
                    dp2[2] *= 1.0f / d2;
                    double cam2FocalPointPosition[3] = { pos2[0] + dp2[0], pos2[1] + dp2[1], pos2[2] + dp2[2] };
                    pDepthWindow->m_spRenderer->GetActiveCamera()->SetFocalPoint(cam2FocalPointPosition);

                    // set the up direction for the depth camera
                    double up2[3] = { 0 };
                    MapVectorFromWorldToCamera(up1, up1, spCameraPosition);
                    MapVectorFromWorldToCamera(up1, up2, m_spDepthCapturer->FirstFisheye.spOrientation);
                    MapVectorFromCameraToWorld(up2, up2, spCameraPosition);

                    d2 = sqrt(up2[0] * up2[0] + up2[1] * up2[1] + up2[2] * up2[2]);
                    up2[0] *= 1.0f / d2;
                    up2[1] *= 1.0f / d2;
                    up2[2] *= 1.0f / d2;
                    pDepthWindow->m_spRenderer->GetActiveCamera()->SetViewUp(up2);
                }

                //Sync the clipping ranges
                double current_clipping_range[2] = { 0 };
                if (pColorWindow)
                {
                    pColorWindow->m_spRenderer->GetActiveCamera()->GetClippingRange(current_clipping_range);
                }

                if (pDepthWindow)
                {
                    pDepthWindow->m_spRenderer->GetActiveCamera()->SetClippingRange(current_clipping_range);
                }

                if (pLFisheyeWindow)
                {
                    pLFisheyeWindow->m_spRenderer->GetActiveCamera()->SetClippingRange(current_clipping_range);
                }

                if (pRFisheyeWindow)
                {
                    pRFisheyeWindow->m_spRenderer->GetActiveCamera()->SetClippingRange(current_clipping_range);
                }

                if (*m_spInputTimer->m_pIsControllerAnimatedTimer)
                {
                    for (uint64_t i = 0; i < static_cast<uint64_t>((m_spInputTimer->m_spTransformInterpolatorCollection).size()); ++i)
                    {
                        auto & pInterpolator = m_spInputTimer->m_spTransformInterpolatorCollection[i];
                        vtkSmartPointer<vtkTransform> spXform = vtkSmartPointer<vtkTransform>::New();
                        pInterpolator->InterpolateTransform(static_cast<double>((m_spInputTimer->m_Min + *m_spInputTimer->m_pFrameIndex * (m_spInputTimer->m_Max - m_spInputTimer->m_Min)) / static_cast<double>(m_spInputTimer->m_TotalNumberFrames)), spXform);
                        auto & pActorCollection = m_spInputTimer->m_spActorCollection[i];
                        pActorCollection->InitTraversal();
                        for (int j = 0; j < pActorCollection->GetNumberOfItems(); ++j)
                        {
                            vtkActor* const pActor = pActorCollection->GetNextActor();
                            pActor->SetUserMatrix(spXform->GetMatrix());
                        }
                    }
                }
                cout << "Index:\t" << *m_spInputTimer->m_pFrameIndex << endl;
            }
            else if (*m_spInputTimer->m_pFrameIndex >= (m_spInputTimer->m_TotalNumberFrames - 1))
            {
                std::cout << "Exiting application." << std::endl;
                (*(m_spInputTimer->m_spInteractorTimer))->TerminateApp();
                return;
            }

            cout << "Camera position.x=" << pColorWindow->m_spRenderer->GetActiveCamera()->GetPosition()[0] << ";y=" << pColorWindow->m_spRenderer->GetActiveCamera()->GetPosition()[1] << ";z=" << pColorWindow->m_spRenderer->GetActiveCamera()->GetPosition()[2] << endl;
            cout << "Camera focal point.x=" << pColorWindow->m_spRenderer->GetActiveCamera()->GetFocalPoint()[0] << ";y=" << pColorWindow->m_spRenderer->GetActiveCamera()->GetFocalPoint()[1] << ";z=" << pColorWindow->m_spRenderer->GetActiveCamera()->GetFocalPoint()[2] << endl;
            cout << "Camera view up.x=" << pColorWindow->m_spRenderer->GetActiveCamera()->GetViewUp()[0] << ";y=" << pColorWindow->m_spRenderer->GetActiveCamera()->GetViewUp()[1] << ";z=" << pColorWindow->m_spRenderer->GetActiveCamera()->GetViewUp()[2] << endl;

            if (pColorWindow)
            {
                pColorWindow->m_spRenderWindow->Render();
                pColorWindow->m_spRenderWindow->WaitForCompletion();
            }

            if (pDepthWindow)
            {
                pDepthWindow->m_spRenderWindow->Render();
                pDepthWindow->m_spRenderWindow->WaitForCompletion();
            }

            if (pLFisheyeWindow)
            {
                pLFisheyeWindow->m_spRenderWindow->Render();
                pLFisheyeWindow->m_spRenderWindow->WaitForCompletion();
            }

            if (pRFisheyeWindow)
            {
                pRFisheyeWindow->m_spRenderWindow->Render();
                pRFisheyeWindow->m_spRenderWindow->WaitForCompletion();
            }

            const unsigned int uTimeout = 100;
            if (m_spSColorWindowCallback && m_spEColorWindowCallback && m_spEDepthWindowCallback)
            {
                while ((*m_spInputTimer->m_pFrameIndex > m_spSColorWindowCallback.GetPointer()->GetLastProcessedFrame()) ||
                    (*m_spInputTimer->m_pFrameIndex > m_spEColorWindowCallback.GetPointer()->GetLastProcessedFrame()) ||
                    (*m_spInputTimer->m_pFrameIndex > m_spEDepthWindowCallback.GetPointer()->GetLastProcessedFrame()))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(uTimeout));
                }
            }
            if (m_spELFisheyeWindowCallback && m_spERFisheyeWindowCallback)
            {
                while ((*m_spInputTimer->m_pFrameIndex > m_spELFisheyeWindowCallback.GetPointer()->GetLastProcessedFrame()) ||
                    (*m_spInputTimer->m_pFrameIndex > m_spERFisheyeWindowCallback.GetPointer()->GetLastProcessedFrame()))
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(uTimeout));
                }
            }
        }
    }
}

void DepthWindowEndCallback::Execute(vtkObject*, unsigned long, void*)
{
    if (m_spInput && *(m_spInput->m_pframeIndex) > lastProcessedFrame && 0 <= *(m_spInput->m_pframeIndex) && *(m_spInput->m_pframeIndex) < m_spInput->m_MaximumFrames)
    {
        if (m_mutex.try_lock())
        {
            if (!m_spDepthCapturer)
            {
                for (const auto& i : m_spInput->m_spCapturers)
                {
                    if (rc_FORMAT_DEPTH16 == i->ImageFormat)
                    {
                        m_spDepthCapturer = i;
                        break;
                    }
                }
            }
            vtkSmartPointer<vtkWindowToImageFilter> spWindowToImage;
            (*(m_spInput->m_spDepthRenderwindow))->RemoveObserver(this);
            // depth
            spWindowToImage = vtkSmartPointer<vtkWindowToImageFilter>::New();
            spWindowToImage->SetInput(*(m_spInput->m_spDepthRenderwindow));
            spWindowToImage->SetInputBufferTypeToZBuffer();
            spWindowToImage->Update();
            vtkSmartPointer<vtkImageShiftScale> spScale = vtkSmartPointer<vtkImageShiftScale>::New();
            spScale->SetOutputScalarTypeToDouble();
            spScale->SetInputConnection(spWindowToImage->GetOutputPort());
            spScale->SetShift(0);
            spScale->Update();
            const double* pDepthPix = reinterpret_cast<double*>(spScale->GetOutput()->GetScalarPointer());
            vtkCamera* cam2 = (*m_spInput->m_spDepthRenderer)->GetActiveCamera();
            double pos2[3] = { 0 };
            cam2->GetPosition(pos2);
            double dp2[3] = { 0 };
            cam2->GetDirectionOfProjection(dp2);
            int dfd_size = sizeof(unsigned short) * m_spDepthCapturer->DepthResolution.width * m_spDepthCapturer->DepthResolution.height * 2;
            std::unique_ptr<unsigned short[]> dfd = std::make_unique<unsigned short[]>(dfd_size);

            for (int i = 0; i < m_spDepthCapturer->DepthResolution.width * m_spDepthCapturer->DepthResolution.height; i++)
            {
                int x = i% m_spDepthCapturer->DepthResolution.width;
                int y = i / m_spDepthCapturer->DepthResolution.width;

                double znb = pDepthPix[i];

                // get z-buffer value
                double z = znb;
                if (z == 1.0)
                {
                    dfd[i] = 0;
                    continue;
                }

                // now convert the display point to world coordinates
                double display[3];
                display[0] = x;
                display[1] = y;
                display[2] = z;
                (*m_spInput->m_spDepthRenderer)->SetDisplayPoint(display);
                (*m_spInput->m_spDepthRenderer)->DisplayToWorld();
                double* world = (*m_spInput->m_spDepthRenderer)->GetWorldPoint();

                // compute vector from camera to point
                for (int j = 0; j < 3; j++)
                {
                    world[j] -= pos2[j];
                }
                z = world[0] * dp2[0] + world[1] * dp2[1] + world[2] * dp2[2];

                // got the depth
                dfd[i] = static_cast<unsigned short>(z * 5000 + 0.5);
                m_max = std::min(m_max, double(dfd[i]));
            }
            Record(dfd.get(), dfd_size, *(m_spInput->m_pframeIndex), m_spDepthCapturer, m_spInput->m_pFramesFile);
            (*(m_spInput->m_spDepthRenderwindow))->AddObserver(vtkCommand::EndEvent, this);
            lastProcessedFrame = m_spInput ? *(m_spInput->m_pframeIndex) : ++lastProcessedFrame;
            m_mutex.unlock();
        }
    }
}

LFisheyeWindowEndCallback* LFisheyeWindowEndCallback::New()
{
    return new LFisheyeWindowEndCallback();
}

RFisheyeWindowEndCallback* RFisheyeWindowEndCallback::New()
{
    return new RFisheyeWindowEndCallback();
}

void RFisheyeWindowEndCallback::Execute(vtkObject*, unsigned long, void*)
{
    if (m_spInput && *(m_spInput->m_pframeIndex) > lastProcessedFrame && 0 <= *(m_spInput->m_pframeIndex) && *(m_spInput->m_pIsFisheyeStereoRecordingEnabled) && *(m_spInput->m_pframeIndex) < m_spInput->m_MaximumFrames)
    {
        if (m_mutex.try_lock())
        {
            for (const auto& i : m_spInput->m_spCapturers)
            {
                if ((rc_FORMAT_DEPTH16 != i->ImageFormat) && (0 != i->CameraIndex))
                {
                    m_spRFisheyeCapturer = i;
                }
            }
            vtkSmartPointer<vtkWindowToImageFilter> spWindowToImage;
            vtkImageData* pImgdata = nullptr;
            (*(m_spInput->m_spRFisheyeRenderwindow))->RemoveObserver(this);
            spWindowToImage = vtkSmartPointer<vtkWindowToImageFilter>::New();
            spWindowToImage->SetInput(*(m_spInput->m_spRFisheyeRenderwindow));
            spWindowToImage->SetInputBufferTypeToRGBA();
            spWindowToImage->Update();
            pImgdata = spWindowToImage->GetOutput();
            const unsigned char* pFisheyePix2 = reinterpret_cast<unsigned char*>(pImgdata->GetScalarPointer());
            int dims[3] = { 0 };
            pImgdata->GetDimensions(dims);
            std::unique_ptr<unsigned char[]> ffd2 = std::make_unique<unsigned char[]>(dims[0] * dims[1] * 4);
            for (int i = 0; i < dims[0] * dims[1] * 4; i += 4)
            {
                ffd2[i] = pFisheyePix2[i];
                ffd2[i + 1] = pFisheyePix2[i + 1];
                ffd2[i + 2] = pFisheyePix2[i + 2];
                ffd2[i + 3] = pFisheyePix2[i + 3];

            }
            m_spRFisheyeCapturer->addDistortion(ffd2.get(), dims, *(m_spInput->m_pframeIndex));
            char fileName[FILE_SIZE_MAX] = { 0 };
            if (0 > snprintf(fileName, sizeof(fileName), RFISHEYE_RELATIVE_PATH, *(m_spInput->m_pframeIndex)))
            {
                cout << "snprintf failed." << "\nLine:" << __LINE__ << "\nFunction:" << __FUNCTION__ << endl;
            }
            *m_spInput->m_pFramesFile << *(m_spInput->m_pframeIndex) << " " << fileName << " " << "\n";
            (*(m_spInput->m_spRFisheyeRenderwindow))->AddObserver(vtkCommand::EndEvent, this);
            lastProcessedFrame = m_spInput ? *(m_spInput->m_pframeIndex) : ++lastProcessedFrame;
            m_mutex.unlock();
        }
    }
}

void LFisheyeWindowEndCallback::Execute(vtkObject*, unsigned long, void*)
{
    if (m_spInput && *(m_spInput->m_pframeIndex) > lastProcessedFrame && 0 <= *(m_spInput->m_pframeIndex)  && *(m_spInput->m_pIsRecordingEnabled) && *(m_spInput->m_pframeIndex) < m_spInput->m_MaximumFrames)
    {
        if (m_mutex.try_lock())
        {
            if (!m_spLFisheyeCapturer)
            {
                for (const auto& i : m_spInput->m_spCapturers)
                {
                    if ((rc_FORMAT_DEPTH16 != i->ImageFormat) && (0 == i->CameraIndex))
                    {
                        m_spLFisheyeCapturer = i;
                        break;
                    }
                }
            }

            (*(m_spInput->m_spLFisheyeRenderwindow))->RemoveObserver(this);
            vtkSmartPointer<vtkWindowToImageFilter> spWindowToImage;
            vtkImageData* pImgdata = nullptr;
            //LFisheye
            spWindowToImage = vtkSmartPointer<vtkWindowToImageFilter>::New();
            spWindowToImage->SetInput(*(m_spInput->m_spLFisheyeRenderwindow));
            spWindowToImage->SetInputBufferTypeToRGBA();
            spWindowToImage->Update();
            pImgdata = spWindowToImage->GetOutput();
            const unsigned char* pFisheyePix = reinterpret_cast<unsigned char*>(pImgdata->GetScalarPointer());
            int dims[3] = { 0 };
            pImgdata->GetDimensions(dims);
            std::unique_ptr<unsigned char[]> ffd = std::make_unique<unsigned char[]>(dims[0] * dims[1] * 4);
            for (int i = 0; i < dims[0] * dims[1] * 4; i += 4)
            {
                ffd[i] = pFisheyePix[i];
                ffd[i + 1] = pFisheyePix[i + 1];
                ffd[i + 2] = pFisheyePix[i + 2];
                ffd[i + 3] = pFisheyePix[i + 3];
            }
            m_spLFisheyeCapturer->addDistortion(ffd.get(), dims, *(m_spInput->m_pframeIndex));
            char fileName[FILE_SIZE_MAX] = { 0 };
            if (0 > snprintf(fileName, sizeof(fileName), LFISHEYE_RELATIVE_PATH, *(m_spInput->m_pframeIndex)))
            {
                cout << "snprintf failed." << "\nLine:" << __LINE__ << "\nFunction:" << __FUNCTION__ << endl;
            }
            *m_spInput->m_pFramesFile << *(m_spInput->m_pframeIndex) << " " << fileName << " ";
            (*(m_spInput->m_spLFisheyeRenderwindow))->AddObserver(vtkCommand::EndEvent, this);
            lastProcessedFrame = m_spInput ? *(m_spInput->m_pframeIndex) : ++lastProcessedFrame;
            m_mutex.unlock();
        }
    }
}

ModifiedCameraPosition* ModifiedCameraPosition::New()
{
    return new ModifiedCameraPosition();
}

void ModifiedCameraPosition::Execute(vtkObject*  pCaller, unsigned long, void*)
{
    vtkCamera* const pCamera = static_cast<vtkCamera* const>(pCaller);
    cout << "\tCamera Position: "
        << pCamera->GetPosition()[0] << ", "
        << pCamera->GetPosition()[1] << ", "
        << pCamera->GetPosition()[2] << std::endl;
    cout << "\tCamera Focal point: "
        << pCamera->GetFocalPoint()[0] << ", "
        << pCamera->GetFocalPoint()[1] << ", "
        << pCamera->GetFocalPoint()[2] << std::endl;
    cout << "\tCamera View Up point: "
        << pCamera->GetViewUp()[0] << ", "
        << pCamera->GetViewUp()[1] << ", "
        << pCamera->GetViewUp()[2] << "\n\n\n";
}