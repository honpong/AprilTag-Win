#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <thread>
#include <mutex>
#include "Common.h"
#include "Utils.h"
#include <new>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <mutex>          
#include "Common.h"

class CSimulatedWindow;
struct CFakeImgCapturer;

struct TimerInput
{
    TimerInput(int * const pFrameIndex, std::vector<vtkSmartPointer<vtkActorCollection>> spActorCollection, unsigned int NumberFrames, std::vector<vtkSmartPointer<vtkTransformInterpolator>> TransformInterpolators, bool* isControllerAnimated, std::vector<std::shared_ptr<CFakeImgCapturer>> spCapturers, std::shared_ptr<CSimulatedWindow> pColorWindow, std::shared_ptr<CSimulatedWindow> pDepthWindow, std::shared_ptr<CSimulatedWindow> pLFisheyeWindow, std::shared_ptr<CSimulatedWindow> pRFisheyeWindow, std::vector<vtkSmartPointer<vtkCameraInterpolator>> spCamerainterp, vtkSmartPointer<vtkRenderWindowInteractor> * pInteractor);
    std::shared_ptr<CSimulatedWindow> m_pColorWindowTimer;
    std::shared_ptr<CSimulatedWindow> m_pDepthWindowTimer;
    std::shared_ptr<CSimulatedWindow> m_pLFisheyeWindowTimer;
    std::shared_ptr<CSimulatedWindow> m_pRFisheyeWindowTimer;
    std::vector<vtkSmartPointer<vtkCameraInterpolator>> m_spCamerainterpTimer;
    std::vector<std::shared_ptr<CFakeImgCapturer>> m_spCapturersTimer; 
    bool* const m_pIsControllerAnimatedTimer;
    std::vector<vtkSmartPointer<vtkTransformInterpolator>> m_spTransformInterpolatorCollection;
    int m_TotalNumberFrames;
    std::vector<vtkSmartPointer<vtkActorCollection>> m_spActorCollection;
    vtkSmartPointer<vtkRenderWindowInteractor> *const m_spInteractorTimer;
    double m_Max;
    double m_Min;
    int * const m_pFrameIndex;
};

class StartCallback : public vtkCommand
{
public:
    vtkTypeMacro(StartCallback, vtkCommand);
    virtual ~StartCallback() = default;
    struct CallbackInput
    {
        CallbackInput(std::vector<std::shared_ptr<std::ofstream>>& spControllerAnimFile, const int* const pframeIndex, const bool* const pIsControllerAnimated, std::vector<vtkSmartPointer<vtkActor>> const spActorSpheres,
            vtkSmartPointer<vtkRenderWindow>* const spColorRenderwindow, vtkSmartPointer<vtkRenderWindow>* const spDepthRenderwindow, vtkSmartPointer<vtkRenderWindow>* const spLFisheyeRenderwindow, vtkSmartPointer<vtkRenderWindow>* const spRFisheyeRenderwindow);
        std::vector<std::shared_ptr<std::ofstream>>& m_spControllerAnimFile;
        const int* const m_pframeIndex;
        const bool* const m_pIsControlleranimated;
        std::vector<vtkSmartPointer<vtkActor>> m_spActorSpheres;
        vtkSmartPointer<vtkRenderWindow>* const m_spColorRenderwindow;
        vtkSmartPointer<vtkRenderWindow>* const m_spDepthRenderwindow;
        vtkSmartPointer<vtkRenderWindow>* const m_spLFisheyeRenderwindow;
        vtkSmartPointer<vtkRenderWindow>* const m_spRFisheyeRenderwindow;
    };
    virtual void InitializeInputs(std::shared_ptr<CallbackInput> spInput) = 0;
    virtual void Execute(vtkObject*, unsigned long, void*);

protected:
    std::shared_ptr<CallbackInput> m_spInput;
    std::mutex m_mutex;
    double lastProcessedFrame;
};

class ColorWindowStartCallback : public StartCallback
{
public:
    vtkTypeMacro(ColorWindowStartCallback, StartCallback);
    virtual void InitializeInputs(std::shared_ptr<CallbackInput> spInput);
    static ColorWindowStartCallback* New();
    double GetLastProcessedFrame() const;
};

class ModifiedCameraPosition : public vtkCommand
{
public:
    vtkTypeMacro(ModifiedCameraPosition, vtkCommand);
    virtual ~ModifiedCameraPosition() = default;
    static ModifiedCameraPosition* New();
    virtual void Execute(vtkObject*, unsigned long, void*);
};

class EndCallback : public vtkCommand
{
public:
    vtkTypeMacro(EndCallback, vtkCommand);
    virtual ~EndCallback() = default;
    struct CallbackInput
    {
        CallbackInput(std::vector<std::shared_ptr<CFakeImgCapturer>> spCapturers, const std::string& szDirectoryName, std::ofstream* const pFramesFile, std::ofstream* const pReferenceFile, std::ofstream* const pAnimationFile, vtkCommand* const pStartCallback, int* const pframeIndex, int MaximumFrames, const bool* const pIsFisheyeStereoRecordingEnabled, const bool* const pIsRecordingEnabled, vtkSmartPointer<vtkRenderer>* const spColorRenderer, vtkSmartPointer<vtkRenderer>* const spDepthRenderer,
            vtkSmartPointer<vtkRenderWindow>* const spColorRenderwindow, vtkSmartPointer<vtkRenderWindow>* const spDepthRenderwindow, vtkSmartPointer<vtkRenderWindow>* const spLFisheyeRenderwindow, vtkSmartPointer<vtkRenderWindow>* const spRFisheyeRenderwindow);

        std::vector<std::shared_ptr<CFakeImgCapturer>> m_spCapturers;
        const std::string m_szDirectoryName;
        std::ofstream* const m_pFramesFile;
        std::ofstream* const m_pReferenceFile;
        std::ofstream* const m_pAnimationFile;
        vtkCommand* const m_pStartCallback;
        int* const m_pframeIndex;
        int m_MaximumFrames;
        const bool* const m_pIsFisheyeStereoRecordingEnabled;
        const bool* const m_pIsRecordingEnabled;
        vtkSmartPointer<vtkRenderer>* const m_spColorRenderer;
        vtkSmartPointer<vtkRenderer>* const m_spDepthRenderer;
        vtkSmartPointer<vtkRenderWindow>* const m_spColorRenderwindow;
        vtkSmartPointer<vtkRenderWindow>* const m_spDepthRenderwindow;
        vtkSmartPointer<vtkRenderWindow>* const m_spLFisheyeRenderwindow;
        vtkSmartPointer<vtkRenderWindow>* const m_spRFisheyeRenderwindow;
    };
    virtual void InitializeInputs(std::shared_ptr<CallbackInput> spInput);
    virtual void Execute(vtkObject*, unsigned long, void*) = 0;
    double GetLastProcessedFrame() const;

protected:
    std::shared_ptr<CallbackInput> m_spInput;
    double m_max;
    vtkSmartPointer<vtkMatrix4x4> m_spInvT0;
    vtkSmartPointer<vtkMatrix4x4> m_spCoordR;
    vtkSmartPointer<vtkMatrix4x4> m_spTmpP;
    vtkSmartPointer<vtkMatrix4x4> m_spTi;
    vtkSmartPointer<vtkMatrix4x4> m_spP;
    vtkSmartPointer<vtkMatrix4x4> m_spTrans2;
    bool m_hasRenderStarted;
    std::mutex m_mutex;
    double lastProcessedFrame;
};

class ColorWindowEndCallback : public EndCallback
{
public:
    vtkTypeMacro(ColorWindowEndCallback, EndCallback);
    virtual void Execute(vtkObject*, unsigned long, void*);
    static ColorWindowEndCallback *New();
    void Record(unsigned char* const cfd, int cfd_size, int currentFrameIndex, std::shared_ptr<CFakeImgCapturer> spcap, const std::string& szDirectoryName, std::ofstream* const pFramesFile);
private:
    std::shared_ptr<CFakeImgCapturer> m_spColorCapturer;
};

class DepthWindowEndCallback : public EndCallback
{
public:
    vtkTypeMacro(DepthWindowEndCallback, EndCallback);
    virtual void Execute(vtkObject*, unsigned long, void*);
    static DepthWindowEndCallback *New();
    void Record(unsigned short* const dfd, int dfd_size, int currentFrameIndex, std::shared_ptr<CFakeImgCapturer> spcap, const std::string& szDirectoryName, std::ofstream* const pFramesFile);
private:
    std::shared_ptr<CFakeImgCapturer> m_spDepthCapturer;
};

class LFisheyeWindowEndCallback : public EndCallback
{
public:
    vtkTypeMacro(LFisheyeWindowEndCallback, EndCallback);
    virtual void Execute(vtkObject*, unsigned long, void*);
    static LFisheyeWindowEndCallback *New();
private:
    std::shared_ptr<CFakeImgCapturer> m_spLFisheyeCapturer;

};

class RFisheyeWindowEndCallback : public EndCallback
{
public:
    vtkTypeMacro(RFisheyeWindowEndCallback, EndCallback);
    virtual void Execute(vtkObject*, unsigned long, void*);
    static RFisheyeWindowEndCallback *New();
private:
    std::shared_ptr<CFakeImgCapturer> m_spRFisheyeCapturer;
};

class TimerWindowCallback : public vtkCommand 
{
public:
    vtkTypeMacro(TimerWindowCallback, vtkCommand);
    void SetInput(const std::shared_ptr<TimerInput>& spInput);
    virtual void Execute(vtkObject*, unsigned long, void*);
    static TimerWindowCallback *New();
    void SetWindowsCallbacks(vtkSmartPointer<ColorWindowStartCallback> spSColorWindowCallback, vtkSmartPointer<ColorWindowEndCallback> spEColorWindowCallback,
        vtkSmartPointer<LFisheyeWindowEndCallback> spELFisheyeWindowCallback, vtkSmartPointer<RFisheyeWindowEndCallback> spERFisheyeWindowCallback,
        vtkSmartPointer<DepthWindowEndCallback> spEDepthWindowCallback);
private:
    std::shared_ptr<TimerInput> m_spInputTimer;
    std::mutex m_mutex;
    vtkSmartPointer<ColorWindowStartCallback> m_spSColorWindowCallback;
    vtkSmartPointer<ColorWindowEndCallback> m_spEColorWindowCallback;
    vtkSmartPointer<LFisheyeWindowEndCallback> m_spELFisheyeWindowCallback;
    vtkSmartPointer<RFisheyeWindowEndCallback> m_spERFisheyeWindowCallback;
    vtkSmartPointer<DepthWindowEndCallback> m_spEDepthWindowCallback;
    std::shared_ptr<CFakeImgCapturer> m_spColorCapturer;
    std::shared_ptr<CFakeImgCapturer> m_spDepthCapturer;
    std::shared_ptr<CFakeImgCapturer> m_spLFisheyeCapturer;
    std::shared_ptr<CFakeImgCapturer> m_spRFisheyeCapturer;
};

#endif
