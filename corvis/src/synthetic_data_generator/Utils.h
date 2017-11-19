#ifndef UTILS_H
#define UTILS_H
#include "Common.h"

struct CFakeImgCapturer;

void MapVectorFromCameraToWorld(const double v[3], double r[3], const vtkSmartPointer<vtkMatrix3x3>& spPosition);
void meshGrid(cv::Mat &ui, cv::Mat &vi, const CFakeImgCapturer& pcap);
void CreateCameraInterpolator(const char* const filename, double viewAngle, vtkSmartPointer<vtkCameraInterpolator>* const spCamerainterp, std::vector<double> * const pTimes);
std::map<std::string, std::string> ParseCmdArgs(int argc, const char* const * argv);
void controllerAnimationRead(const char* const filename, int* const pControllerAnimationSize, std::vector<std::vector<double>>* const pControllerAnim);
void MapVectorFromWorldToCamera(const double v[3], double r[3], const vtkSmartPointer<vtkMatrix3x3>& spPosition);
void PrintUsage();

#endif // !UTILS_H

