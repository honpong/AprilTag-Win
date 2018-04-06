#ifndef COMMON_H
#define COMMON_H
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <algorithm>
#include <map>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstddef>
#include <limits>
#include <memory>
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <math.h>
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include <thread>
#include <future>
#include <chrono>
#include "VTKCommonHeaders.h"
#include "TrackerProxy.h"

#define FILE_SIZE_MAX          64U
#define COLOR_RELATIVE_PATH    "color%04d.png"
#define DEPTH_RELATIVE_PATH    "depth%04d.png"
#define LFISHEYE_RELATIVE_PATH "leftFisheye%04d.png"
#define RFISHEYE_RELATIVE_PATH "rightFisheye%04d.png"

#ifdef WIN32
#include <direct.h>
#define mkdir(dir,mode) _mkdir(dir)
#else
#include <sys/stat.h>
#endif

struct Resolution
{
    uint16_t width;
    uint16_t height;
};

struct MatrixParameters
{
    double TranslationParameters[3];
    vtkSmartPointer<vtkMatrix3x3> spOrientation;
};

struct CFakeImgCapturer
{
    CFakeImgCapturer();
    virtual ~CFakeImgCapturer() = default;
    virtual void distortionEquation(cv::Mat* const pxDistorted, cv::Mat* const pyDistorted) = 0;
    virtual void InitializeCapturer() = 0;
    virtual void validateFisheyeDistortion(const std::string& szDirectoryName);
    virtual void addDistortion(unsigned char* ffd_img, int dims[3], int currentFrameIndex);
    virtual void fisheyeRenderSizeCompute();
    virtual void SetCameraIntrinsics(const rc_ExtendedCameraIntrinsics& camera);
    virtual void PrintCapturer() const;
    virtual void SetDirectoryName(const std::string& szDirectory);
    virtual std::string GetDirectoryName() const;

    MatrixParameters FirstFisheye;
    MatrixParameters SecondFisheye;
    Resolution ColorResolution;
    Resolution DepthResolution;
    Resolution InputFisheyeResolution;
    vtkSmartPointer<vtkCoordinate> spColorFocalLength;
    vtkSmartPointer<vtkCoordinate> spDepthFocalLength;
    vtkSmartPointer<vtkCoordinate> spColorFOV;
    vtkSmartPointer<vtkCoordinate> spDepthFOV;
    vtkSmartPointer<vtkCoordinate> spInputFisheyeFOV;
    vtkSmartPointer<vtkCoordinate> spInputC;
    vtkSmartPointer<vtkCoordinate> spInputFisheyeFocalLength;
    double FisheyeFOV;
    int MaxIR;
    std::shared_ptr<cv::Mat> spXd;
    std::shared_ptr<cv::Mat> spYd;
    rc_CameraIntrinsics CameraIntrinsics;
    rc_ImageFormat ImageFormat;
    uint8_t CameraIndex;
    std::string m_szDirectoryPath;
};

struct FOVCapturer : public CFakeImgCapturer
{
    virtual void InitializeCapturer();
    virtual void distortionEquation(cv::Mat* const pxDistorted, cv::Mat* const pyDistorted);
};

struct KB4Capturer : public CFakeImgCapturer
{
    virtual void InitializeCapturer();
    virtual void distortionEquation(cv::Mat* const pxDistorted, cv::Mat* const pyDistorted);
};

struct RGBColor
{
    double R;
    double G;
    double B;
};

#endif // !COMMON_H

