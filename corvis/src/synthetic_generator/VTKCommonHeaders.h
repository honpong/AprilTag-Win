#ifndef VTKCOMMONHEADERS_H
#define VTKCOMMONHEADERS_H

#define vtkRenderingCore_AUTOINIT 3(vtkInteractionStyle,vtkRenderingFreeType,vtkRenderingOpenGL2)
#define vtkRenderingVolume_AUTOINIT 1(vtkRenderingVolumeOpenGL2)

#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkCommand.h>
#include <vtkCallbackCommand.h>
#include <vtkCameraInterpolator.h>
#include <vtkCellArray.h>
#include <vtkCylinderSource.h>
#include <vtkImageShiftScale.h>
#include <vtkImageCast.h>
#include <vtkImageImport.h>
#include <vtkInteractorStyleTrackballCamera.h>
#include <vtkInteractorStyleFlight.h>
#include <vtkMultiThreader.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkOBJReader.h>
#include <vtkProperty.h>
#include <vtkPolyData.h>
#include <vtkPoints.h>
#include <vtkPolyDataMapper.h>
#include <vtkPNGWriter.h>
#include <vtkPointData.h>
#include <vtkPLYReader.h>
#include <vtkPNGReader.h>
#include <vtkPolyDataNormals.h>
#include <vtkPlaneSource.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRendererCollection.h>
#include <vtkRenderLargeImage.h>
#include <vtkRegularPolygonSource.h>
#include <vtkSphereSource.h>
#include <vtkTexture.h>
#include <vtkTransform.h>
#include <vtkUnsignedCharArray.h>
#include <vtkVector.h>
#include <vtkVersion.h>
#include <vtkVertexGlyphFilter.h>
#include <vtkWorldPointPicker.h>
#include <vtkInteractorStyleJoystickCamera.h>
#include <vtkWindowToImageFilter.h>
#include "vtkObjectFactory.h"
#include "vtkInformationVector.h"
#include "vtkInformation.h"
#include "vtkDataObject.h"
#include "vtkSmartPointer.h"
#include <vtkParametricFunctionSource.h>
#include <vtkTupleInterpolator.h>
#include <vtkTubeFilter.h>
#include <vtkParametricSpline.h>
#include <vtkQuaternion.h>
#include <vtkTransformInterpolator.h>
#include <vtkCoordinate.h>
#include <vtkRenderWindowCollection.h>
#include <vtkMatrix3x3.h>
#include <vtkVector.h>
#include <vtkMath.h>
#include <vtkImageReader2Factory.h>
#include <vtkSTLReader.h>
// For compatibility with new VTK generic data arrays
#ifdef vtkGenericDataArray_h
#define InsertNextTupleValue InsertNextTypedTuple
#endif

#endif // !VTKCOMMONHEADERS_H

