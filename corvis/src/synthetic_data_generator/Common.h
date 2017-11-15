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

#define COLOR_RELATIVE_PATH    "color%04d.png"
#define DEPTH_RELATIVE_PATH    "depth%04d.png"
#define LFISHEYE_RELATIVE_PATH "leftFisheye%04d.png"
#define RFISHEYE_RELATIVE_PATH "rightFisheye%04d.png"

#ifdef _WIN32
#define COLOR_FILE_PATH         (std::string("\\color\\") + std::string(COLOR_RELATIVE_PATH))
#define DEPTH_FILE_PATH         (std::string("\\depth\\") + std::string(DEPTH_RELATIVE_PATH))
#define LFISHEYE_FILE_PATH      (std::string("\\fisheye_0\\") + std::string(LFISHEYE_RELATIVE_PATH))
#define RFISHEYE_FILE_PATH      (std::string("\\fisheye_1\\") + std::string(RFISHEYE_RELATIVE_PATH))
#else
#define COLOR_FILE_PATH         (std::string("/color/") + std::string(COLOR_RELATIVE_PATH))
#define DEPTH_FILE_PATH         (std::string("/depth/") + std::string(DEPTH_RELATIVE_PATH))
#define LFISHEYE_FILE_PATH      (std::string("/fisheye_0/") + std::string(LFISHEYE_RELATIVE_PATH))
#define RFISHEYE_FILE_PATH      (std::string("/fisheye_1/") + std::string(RFISHEYE_RELATIVE_PATH))
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
    virtual void distortionEquation(cv::Mat &xDistorted, cv::Mat &yDistorted) = 0;
    virtual void InitializeCapturer() = 0;
    virtual void validateFisheyeDistortion(const std::string& szDirectoryName);
    virtual void addDistortion(unsigned char* ffd_img, int dims[3], int currentFrameIndex, const char* filename, const std::string& szDirectoryName);
    virtual void fisheyeRenderSizeCompute();
    virtual void SetCameraIntrinsics(const rc_ExtendedCameraIntrinsics& camera);
    virtual void PrintCapturer() const;
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
};

struct FOVCapturer : public CFakeImgCapturer
{
    virtual void InitializeCapturer();
    virtual void distortionEquation(cv::Mat &xDistorted, cv::Mat &yDistorted);
};

struct KB4Capturer : public CFakeImgCapturer
{
    virtual void InitializeCapturer();
    virtual void distortionEquation(cv::Mat &xDistorted, cv::Mat &yDistorted);
};

struct RGBColor
{
    double R;
    double G;
    double B;
};

#endif // !COMMON_H

