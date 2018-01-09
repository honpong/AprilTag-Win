#ifndef  SCENARIOS_H
#define  SCENARIOS_H

#include "Common.h"
#include "ScenesObjects.h"
#include "Callbacks.h"

class CMesh;
struct TimerInput;
class CActorSphere;
class CActorCylinder;
class TimerWindowCallback;
class ModifiedCameraPosition;
class DepthWindowEndCallback;
class ColorWindowEndCallback;
class ColorWindowStartCallback;
class LFisheyeWindowEndCallback;
class RFisheyeWindowEndCallback;
class TrackerProxy;

class IVTKScenario
{
public:
    virtual ~IVTKScenario() = default;
    virtual void Execute() = 0;
    virtual size_t InitializeSetup(std::map<std::string, std::string> args) = 0;
    virtual void SetupActorsInWindows(vtkRenderer* const &pRenderer, vtkRenderWindow* const & pWindow);
    virtual size_t LoadCalibrationFile(std::map<std::string, std::string> args);

protected:
    std::map<std::string, std::string> m_args;
    std::vector<std::vector<std::vector<double>>> m_controllerAnim;
    vtkSmartPointer<TimerWindowCallback> m_spTimerWindowCallback;
    std::shared_ptr<TimerInput> m_spTimerInput;
    std::vector<vtkSmartPointer<vtkActorCollection>> m_spActorCollection;
    int m_frameIndex;
    unsigned long m_NumberOfFrames;
    unsigned long m_RenderingTimeInterval;
    bool m_isControllerAnimated;
    std::shared_ptr<CSimulatedWindow> m_spColorWindow;
    std::shared_ptr<CSimulatedWindow> m_spDepthWindow;
    std::shared_ptr<CSimulatedWindow> m_spLFisheyeWindow;
    std::shared_ptr<CSimulatedWindow> m_spRFisheyeWindow;
    std::shared_ptr<CFakeImgCapturer> m_spColorCapturer;
    std::shared_ptr<CFakeImgCapturer> m_spDepthCapturer;
    std::shared_ptr<CFakeImgCapturer> m_spLFisheyeCapturer;
    std::shared_ptr<CFakeImgCapturer> m_spRFisheyeCapturer;
    std::vector<std::shared_ptr<CFakeImgCapturer>> m_spCapturers;
    std::vector<vtkSmartPointer<vtkCameraInterpolator>> m_spCamerainterp;
    std::vector<vtkSmartPointer<vtkTransformInterpolator>> m_spTransformInterpolatorCollection;
    std::vector<vtkSmartPointer<vtkRenderWindowInteractor>> m_spInteractors;
    std::vector<int> m_controllerAnimationSize;
    vtkSmartPointer<vtkPolyDataMapper> m_spMapper;
    vtkSmartPointer<vtkPolyDataNormals> m_spNorms;
    std::shared_ptr<CMesh> m_spMesh;
    bool m_hasTexture;
    vtkSmartPointer<vtkTexture> m_spTexture;
    std::unique_ptr<TrackerProxy> m_spProxy;
    std::string m_szCalibrationContents;
};

class CBannerSimpleScenario : public IVTKScenario
{
public:
    enum class WindowIndex { COLOR = 0, DEPTH = 1, LFISHEYE = 2, RFISHEYE = 3 };
    virtual void Execute() override;
    virtual size_t InitializeSetup(std::map<std::string, std::string> args) override;
    CBannerSimpleScenario();
    ~CBannerSimpleScenario() = default;
    size_t ParseUserInput(std::map<std::string, std::string> args);
    void CreateCameraWindows();
    size_t AddControllerInWindow(vtkSmartPointer<vtkCoordinate> spControllerCenterCoordinates, uint64_t uIndex);
    bool isControllerAnimated() const;
    void GenerateInterpolations(std::string ControllerAnimationFile, vtkSmartPointer<vtkTransformInterpolator> * const spTransformInterpolator);
    void StartScenario();
    void CreateTimerWindowCallback();
    void AddNewControllerInterpolation(vtkSmartPointer<vtkTransformInterpolator> spNewInterpolation);
    bool isAnimationEnabled() const;
    void CreateCamerasCallbacks();

protected:
    friend TimerWindowCallback;
    void HandleDirectoryCreation();
	size_t HandleControllerFile(const vtkSmartPointer<vtkCoordinate>& spOrbCenterCoordinates, uint64_t uIndex);

    enum
    {
        DEFAULT_RENDERING_TIME_INTERVAL = 100,
        DEFAULT_NUMBER_OF_INTERPOLATION_POINTS = 100
    };

    vtkSmartPointer<vtkRendererCollection> m_spRendererCollection;
    vtkSmartPointer<ColorWindowStartCallback> m_spSColorWindowCallback;
    vtkSmartPointer<ColorWindowEndCallback> m_spEColorWindowCallback;
    vtkSmartPointer<LFisheyeWindowEndCallback> m_spELFisheyeWindowCallback;
    vtkSmartPointer<RFisheyeWindowEndCallback> m_spERFisheyeWindowCallback;
    vtkSmartPointer<DepthWindowEndCallback> m_spEDepthWindowCallback;
    bool m_isAnimationEnabled;
    bool m_isRecordingEnabled;
    bool m_isFisheyeStereoRecordingEnabled;
    std::string m_szDirectoryName;
    std::vector<std::shared_ptr<std::ofstream>> m_spControllerAnimFile;
    std::ofstream m_framesFile;
    std::ofstream m_referenceFile;
    std::ofstream m_animationFile;
};

class CBannerTwoControllersScenario : public IVTKScenario
{
public:
    virtual void Execute() override;
    virtual size_t InitializeSetup(std::map<std::string, std::string> args) override;
    CBannerTwoControllersScenario();
    ~CBannerTwoControllersScenario() = default;

protected:
    std::vector<std::vector<double>> m_secondControllerAnim;
    std::shared_ptr<CBannerSimpleScenario> m_spBannerMainScenario;
};

class CInteractiveScenario : public IVTKScenario
{
public:
    virtual void Execute() override;
    virtual size_t InitializeSetup(std::map<std::string, std::string> args) override;
    ~CInteractiveScenario() = default;
private:
    vtkSmartPointer<ModifiedCameraPosition> m_spCameraPosition;
};

#endif // ! SCENARIOS_H