#include <thread>
#include <mutex>
#include "Utils.h"
#include <new>
#include <iostream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include "ScenesObjects.h"

CMesh::CMesh(std::map<std::string, std::string> args) : m_args(args) {}

void CMesh::Create()
{
    // compute the normals
    m_spNorms = vtkSmartPointer<vtkPolyDataNormals>::New();
    m_spNorms->ConsistencyOn();

    //load the mesh
    std::string meshfile = m_args["--mesh"];
    std::string extension = meshfile.substr(meshfile.length() - 4);
    if (extension == std::string(".obj"))
    {
        vtkSmartPointer<vtkOBJReader> spObjReader = vtkSmartPointer<vtkOBJReader>::New();
        spObjReader->SetFileName(meshfile.c_str());
        m_spMeshReader = spObjReader;
    }
    else if (extension == std::string(".ply"))
    {
        vtkSmartPointer<vtkPLYReader> spPlyReader = vtkSmartPointer<vtkPLYReader>::New();
        spPlyReader->SetFileName(meshfile.c_str());
        m_spMeshReader = spPlyReader;
    }
    else
    {
        PrintUsage();
        return;
    }
    m_spNorms->SetInputConnection(m_spMeshReader->GetOutputPort());
}

void CSimulatedWindow::Create()
{
    m_spRenderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    m_spRenderer = vtkSmartPointer<vtkRenderer>::New();
    m_spRenderer->UseFXAAOn();
    m_spRenderer->SetBackground(m_Background.R, m_Background.G, m_Background.B);
    m_spRenderer->GetActiveCamera()->SetViewAngle(m_viewAngle);
    m_spRenderer->GetActiveCamera()->SetParallelProjection(m_ProjectionValue);
    m_spRenderer->SetTwoSidedLighting(m_LightingValue);
    m_spRenderWindow->AddRenderer(m_spRenderer);
    m_spRenderWindow->SetWindowName(m_szName.c_str());
    m_spRenderWindow->SetPosition(m_spPosition->GetValue()[0], m_spPosition->GetValue()[1]);
    m_spRenderWindow->SetSize(m_uWidth, m_uHeight);
}

CSimulatedWindow::CSimulatedWindow(unsigned int uWidth, unsigned int uHeight, const vtkSmartPointer<vtkCoordinate>& spPosition, double viewAngle, int projectionValue, const RGBColor& Background, int LightingValue, std::string szName) :
    m_uWidth(uWidth), m_uHeight(uHeight), m_spPosition(spPosition), m_viewAngle(viewAngle), m_ProjectionValue(projectionValue), m_Background(Background), m_LightingValue(LightingValue), m_szName(szName) {}

void CActorSphere::Create()
{
    m_spPolygonSphereSource = vtkSmartPointer<vtkSphereSource>::New();
    m_spPolygonSphereSource->SetCenter(m_spCenterCoordinates->GetValue()[0], m_spCenterCoordinates->GetValue()[1], m_spCenterCoordinates->GetValue()[2]);
    m_spPolygonSphereSource->SetRadius(m_radius);
    m_spPolygonSphereSource->SetThetaResolution(m_thetaResolution);
    m_spPolygonSphereSource->SetPhiResolution(m_phiResolution);
    m_spPolygonSphereSource->Update();
}

CActorSphere::CActorSphere(const vtkSmartPointer<vtkCoordinate>& spCenterCoordinates, double radius, double ThetaResolution, double PhiResolution) :
    m_spCenterCoordinates(spCenterCoordinates), m_radius(radius), m_thetaResolution(ThetaResolution), m_phiResolution(PhiResolution)
{
}

void CActorCylinder::Create()
{
    m_spCylinderSource = vtkSmartPointer<vtkCylinderSource>::New();
    m_spCylinderSource->SetCenter(m_spCenterCoordinates->GetValue()[0], m_spCenterCoordinates->GetValue()[1], m_spCenterCoordinates->GetValue()[2]);
    m_spCylinderSource->SetRadius(m_radius);
    m_spCylinderSource->SetHeight(m_Height);
    m_spCylinderSource->SetResolution(m_Resolution);
    m_spCylinderSource->Update();
}

CActorCylinder::CActorCylinder(const vtkSmartPointer<vtkCoordinate>& spCenterCoordinates, double radius, double Resolution, double Height) :
    m_spCenterCoordinates(spCenterCoordinates), m_radius(radius), m_Resolution(Resolution), m_Height(Height) {}
