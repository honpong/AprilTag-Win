#ifndef SCENESOBJECTS_H
#define SCENESOBJECTS_H

#include "VTKCommonHeaders.h"
#include "Common.h"

struct RGBColor;

class IVirtualObject
{
    public:
        virtual ~IVirtualObject() = default;
        virtual void Create() = 0;
};

class CActorSphere : public IVirtualObject
{
public:
    CActorSphere(const vtkSmartPointer<vtkCoordinate>& spCenterCoordinates, double radius, double ThetaResolution, double PhiResolution);
    virtual void Create();

private:
    friend class CBannerSimpleScenario;

    vtkSmartPointer<vtkActor> m_spActorSphere;
    vtkSmartPointer<vtkSphereSource> m_spPolygonSphereSource;
    vtkSmartPointer<vtkCoordinate> m_spCenterCoordinates;
    double m_radius;
    double m_thetaResolution;
    double m_phiResolution;
};

class CActorCylinder : public IVirtualObject
{
public:
    CActorCylinder(const vtkSmartPointer<vtkCoordinate>& spCenterCoordinates, double radius, double Resolution, double Height);
    virtual void Create();

private:
    friend class CBannerSimpleScenario;

    vtkSmartPointer<vtkActor> m_spActorCylinder;
    vtkSmartPointer<vtkCylinderSource> m_spCylinderSource;
    vtkSmartPointer<vtkCoordinate>  m_spCenterCoordinates;
    double m_radius;
    double m_Height;
    double m_Resolution;
};

class CMesh : public IVirtualObject
{
public:
    CMesh(std::map<std::string, std::string> args);
    virtual void Create();

private:
    friend class IVTKScenario;

    std::map<std::string, std::string> m_args;
    vtkSmartPointer<vtkPolyDataNormals> m_spNorms;
    vtkSmartPointer<vtkPolyDataAlgorithm> m_spMeshReader;
};

class CSimulatedWindow : public IVirtualObject
{
public:
    CSimulatedWindow(unsigned int uWidth, unsigned int uHeight, const vtkSmartPointer<vtkCoordinate>& spPosition, double viewAngle, int projectionValue, const RGBColor& Background, int LightingValue, std::string szName);
    virtual void Create();

private:
    friend class CBannerSimpleScenario;
    friend class TimerWindowCallback;
    friend class CInteractiveScenario;

    vtkSmartPointer<vtkRenderer> m_spRenderer;
    vtkSmartPointer<vtkRenderWindow> m_spRenderWindow;
    vtkSmartPointer<vtkCoordinate> m_spPosition;
    int m_ProjectionValue;
    int m_LightingValue;
    unsigned int m_uWidth;
    unsigned int m_uHeight;
    double m_viewAngle;
    RGBColor m_Background;
    std::string m_szName;
};

#endif // ! SCENESOBJECTS_H
