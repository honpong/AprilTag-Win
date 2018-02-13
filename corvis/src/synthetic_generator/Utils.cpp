#include <thread>
#include <mutex>
#include "Common.h"
#include <future>
#include <algorithm>

inline double getFOV(double imageDimension, double focalLength)
{
    return ((2 * std::atan(imageDimension / (2 * focalLength)) * 180 / M_PI));
}

void MapVectorFromWorldToCamera(const double v[3], double r[3], const vtkSmartPointer<vtkMatrix3x3>& spPosition)
{
    const unsigned int uSize = 3;
    double temp[uSize][uSize] = {};
    int k = 0;
    for (int i = 0; i < uSize; ++i)
    {
        for (int j = 0; j < uSize; ++j)
        {
            temp[i][j] = spPosition->GetData()[k++];
        }
    }
    vtkMath::Multiply3x3(temp, v, r);
}

void MapVectorFromCameraToWorld(const double v[3], double r[3], const vtkSmartPointer<vtkMatrix3x3>& spPosition)
{
    vtkSmartPointer<vtkMatrix3x3> sp = spPosition;
    sp->Transpose();
    MapVectorFromWorldToCamera(v, r, sp);
    sp->Transpose();
}

//Creates a 2D mesh equivalent to the size of desired fisheye image
void meshGrid(cv::Mat* const pUi, cv::Mat* const pVi, const CFakeImgCapturer& pcap)
{
    for (uint64_t i = 0; i < pcap.CameraIntrinsics.height_px; i++)
    {
        for (uint64_t j = 0; j < pcap.CameraIntrinsics.width_px; ++j) {
            (*pUi).at<float>(i, j) = (float)j;
        }
    }
    for (uint64_t i = 0; i < pcap.CameraIntrinsics.width_px; i++)
    {
        for (uint64_t j = 0; j < pcap.CameraIntrinsics.height_px; ++j) {
            (*pVi).at<float>(j, i) = (float)j;
        }
    }
}

void KB4Capturer::distortionEquation(cv::Mat* const pxDistorted, cv::Mat* const pyDistorted)
{
    cv::Mat ui = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1, 0.0);
    cv::Mat vi = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1, 0.0);
    cv::Mat ru = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1, 0.0);
    cv::Mat xd = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1, 0.0);
    cv::Mat yd = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1, 0.0);
    cv::Mat rd = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1, 0.0);
    cv::Mat thetaImage = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1, 0.0);
    cv::Mat ku_d = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1, 0.0);

    meshGrid(&ui, &vi,*this);
    for (uint64_t i = 0; i < CameraIntrinsics.height_px; ++i)
    {
        for (uint64_t j = 0; j < CameraIntrinsics.width_px; ++j) {
            ui.at<float>(i, j) = (ui.at<float>(i, j) - CameraIntrinsics.c_x_px) / CameraIntrinsics.f_x_px;
            vi.at<float>(i, j) = (vi.at<float>(i, j) - CameraIntrinsics.c_y_px) / CameraIntrinsics.f_y_px;
            rd.at<float>(i, j) = sqrt(ui.at<float>(i, j) *ui.at<float>(i, j) + vi.at<float>(i, j) *vi.at<float>(i, j));
            if (rd.at<float>(i, j) < FLT_EPSILON)
            {
                rd.at<float>(i, j) = FLT_EPSILON;
            }
            float theta = rd.at<float>(i, j);
            //if (std::abs(tanArg - MY_PI / 2.0) < 0.0925)

            float theta2 = theta*theta;
            for (int l = 0; l < 4; l++)
            {
                float f_x = theta*(1 + theta2*(CameraIntrinsics.k1 + theta2*(CameraIntrinsics.k2 + theta2*(CameraIntrinsics.k3 + theta2*CameraIntrinsics.k4)))) - rd.at<float>(i, j);
                if (f_x == 0)
                {
                    break;
                }
                float der_f_x = 1 + theta2*(3 * CameraIntrinsics.k1 + theta2*(5 * CameraIntrinsics.k2 + theta2*(7 * CameraIntrinsics.k3 + 9 * theta2*CameraIntrinsics.k4)));
                theta -= f_x / der_f_x;
                theta2 = theta*theta;
            }

            if (theta > M_PI_2 - 0.09)
            {
                theta = 0;
            }
            thetaImage.at<float>(i, j) = theta;
            ru.at<float>(i, j) = tan(theta);
            ku_d.at<float>(i, j) = ru.at<float>(i, j) / rd.at<float>(i, j);
            xd.at<float>(i, j) = ui.at<float>(i, j) * ku_d.at<float>(i, j);
            yd.at<float>(i, j) = vi.at<float>(i, j) * ku_d.at<float>(i, j);
            (*pxDistorted).at<float>(i, j) = xd.at<float>(i, j) * spInputFisheyeFocalLength->GetValue()[0];
            (*pyDistorted).at<float>(i, j) = yd.at<float>(i, j) * spInputFisheyeFocalLength->GetValue()[1];
        }
    }
    (*pxDistorted).at<float>(CameraIntrinsics.c_y_px, CameraIntrinsics.c_x_px) = 0;
    (*pyDistorted).at<float>(CameraIntrinsics.c_y_px, CameraIntrinsics.c_x_px) = 0;
}


//FOV model-Inverse Distortion Equation
void FOVCapturer::distortionEquation(cv::Mat* const pxDistorted, cv::Mat* const pyDistorted)
{
    cv::Mat ui = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1, 0.0);
    cv::Mat vi = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1, 0.0);
    cv::Mat rd = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1, 0.0);
    cv::Mat ru = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1, 0.0);
    cv::Mat xu = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1, 0.0);
    cv::Mat yu = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1, 0.0);

    meshGrid(&ui, &vi, *this);
    for (uint64_t i = 0; i < CameraIntrinsics.height_px; ++i)
    {
        for (uint64_t j = 0; j < CameraIntrinsics.width_px; ++j) {
            ui.at<float>(i, j) = (ui.at<float>(i, j) - CameraIntrinsics.c_x_px) / CameraIntrinsics.f_x_px;
            vi.at<float>(i, j) = (vi.at<float>(i, j) - CameraIntrinsics.c_y_px) / CameraIntrinsics.f_y_px;
            rd.at<float>(i, j) = sqrt(ui.at<float>(i, j) *ui.at<float>(i, j) + vi.at<float>(i, j) *vi.at<float>(i, j));
            float tanArg = rd.at<float>(i, j) * CameraIntrinsics.w;
            if (tanArg >(M_PI / 2.0 - 0.0765))
            {
                tanArg = 0;
            }
            ru.at<float>(i, j) = tan(tanArg) / (2 * tan(CameraIntrinsics.w / 2));
            xu.at<float>(i, j) = ui.at<float>(i, j) * ru.at<float>(i, j) / rd.at<float>(i, j);
            yu.at<float>(i, j) = vi.at<float>(i, j) * ru.at<float>(i, j) / rd.at<float>(i, j);
            (*pxDistorted).at<float>(i, j) = xu.at<float>(i, j) * spInputFisheyeFocalLength->GetValue()[0];
            (*pyDistorted).at<float>(i, j) = yu.at<float>(i, j) * spInputFisheyeFocalLength->GetValue()[1];
        }
    }
    (*pxDistorted).at<float>(CameraIntrinsics.c_y_px, CameraIntrinsics.c_x_px) = 0;
    (*pyDistorted).at<float>(CameraIntrinsics.c_y_px, CameraIntrinsics.c_x_px) = 0;
}

void CFakeImgCapturer::SetDirectoryName(const std::string& szDirectory)
{
    m_szDirectoryPath = szDirectory;
}


//Computes the maximum renderer window size required to get the desired output distorted image.
void CFakeImgCapturer::fisheyeRenderSizeCompute()
{
    cv::Mat xd = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1);
    cv::Mat yd = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1);
    distortionEquation(&xd, &yd);
    float CxCompute = round(-xd.at<float>(0, 0));
    float CyCompute = round(-yd.at<float>(0, 0));
    if (CxCompute < 0 && CyCompute < 0)
    {
        CxCompute *= -1;
        CyCompute *= -1;
    }
    else if (CxCompute < 0)
    {
        CxCompute *= -1;
    }
    else if (CyCompute < 0)
    {
        CyCompute *= -1;
    }

    cv::Mat xdCompute = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1);
    cv::Mat ydCompute = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1);
    float max_x = 0.00;
    float max_y = 0.00;
    for (uint64_t i = 0; i < CameraIntrinsics.height_px; i++)
    {
        for (uint64_t j = 0; j < CameraIntrinsics.width_px; ++j) {
            xdCompute.at<float>(i, j) = xd.at<float>(i, j) + CxCompute;
            ydCompute.at<float>(i, j) = yd.at<float>(i, j) + CyCompute;
            if (xdCompute.at<float>(i, j) > max_x)
            {
                max_x = xdCompute.at<float>(i, j);
            }
            if (ydCompute.at<float>(i, j) > max_y)
            {
                max_y = ydCompute.at<float>(i, j);
            }
        }
    }
    InputFisheyeResolution.width = round(max_x);
    InputFisheyeResolution.height = round(max_y);
    spInputC->SetValue((InputFisheyeResolution.width / 2.0) - 0.5, (InputFisheyeResolution.height / 2.0) - 0.5);
    spInputFisheyeFOV->SetValue((2 * std::atan(InputFisheyeResolution.width / (2 * spInputFisheyeFocalLength->GetValue()[0])) * 180 / M_PI),(2 * std::atan(InputFisheyeResolution.height / (2 * spInputFisheyeFocalLength->GetValue()[1])) * 180 / M_PI));
}

void CFakeImgCapturer::PrintCapturer() const
{
    // Printing camera intrinsics.
    std::cout << "CameraIntrinsics: \n\tc_x_px: " << CameraIntrinsics.c_x_px << std::endl;
    std::cout << "                  \n\tc_y_px: " << CameraIntrinsics.c_y_px << std::endl;
    for (uint8_t i = 0; i <sizeof(CameraIntrinsics.distortion)/sizeof(CameraIntrinsics.distortion[0]); ++i)
    {
        std::cout << "                  \n\tdistortion:index " << static_cast<int>(i) <<" value " <<  CameraIntrinsics.distortion[i] << std::endl;
    }
    std::cout << "                  \n\tf_x_px: " << CameraIntrinsics.f_x_px << std::endl;
    std::cout << "                  \n\tf_y_px: " << CameraIntrinsics.f_y_px << std::endl;
    std::cout << "                  \n\twidth_px: " << CameraIntrinsics.width_px << std::endl;
    std::cout << "                  \n\theight_px: " << CameraIntrinsics.height_px << std::endl;
    std::cout << "                  \n\tk1: " << CameraIntrinsics.k1 << std::endl;
    std::cout << "                  \n\tk2: " << CameraIntrinsics.k2 << std::endl;
    std::cout << "                  \n\tk3: " << CameraIntrinsics.k3 << std::endl;
    std::cout << "                  \n\tk4: " << CameraIntrinsics.k4 << std::endl;
    std::cout << "                  \n\ttype: " << static_cast<int>(CameraIntrinsics.type) << std::endl;
    std::cout << "                  \n\tw: " << CameraIntrinsics.w << std::endl;
}

std::string CFakeImgCapturer::GetDirectoryName() const
{
    return m_szDirectoryPath;
}

//Remaps the input image buffer to the distorted coordinates to get the fisheye distorted image
void CFakeImgCapturer::addDistortion(unsigned char* ffd_img, int dims[3], int currentFrameIndex)
{
    cv::Mat TempMat = cv::Mat(dims[1], dims[0], CV_8UC4);
    TempMat.data = ffd_img;
    cvtColor(TempMat, TempMat, CV_RGBA2GRAY);

    if (!spXd || !spYd)
    {
        spXd = std::make_shared<cv::Mat>(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1);
        spYd = std::make_shared<cv::Mat>(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1);
        distortionEquation(spXd.get(), spYd.get());
        for (uint64_t i = 0; i < CameraIntrinsics.height_px; ++i)
        {
            for (uint64_t j = 0; j < CameraIntrinsics.width_px; ++j)
            {
                (*spXd).at<float>(i, j) = (*spXd).at<float>(i, j) + spInputC->GetValue()[0];
                (*spYd).at<float>(i, j) = (*spYd).at<float>(i, j) + spInputC->GetValue()[1];
            }
        }
    }

    cv::Mat res = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_8UC1);
    remap(TempMat, res, *spXd, *spYd, cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(1, 0, 0));
    res.at<uchar>(0, 0) = TempMat.at<uchar>(0, 0);
    char fileName[FILE_SIZE_MAX] = { 0 };
    if(0 > snprintf(fileName, sizeof(fileName), m_szDirectoryPath.c_str(), currentFrameIndex))
    {
        cout << "snprintf failed." << "\nLine:" << __LINE__ << "\nFunction:" << __FUNCTION__ << endl;
    }
    transpose(res, res);
    flip(res, res, 1);
    transpose(res, res);
    imwrite(fileName, res);
}

//Validates the distortion by comapring the saved renderer input image to the undistorted image
void CFakeImgCapturer::validateFisheyeDistortion(const std::string& szDirectoryName) 
{
    std::string validationImagePath = szDirectoryName + "FisheyeValidation0000.png";
    cv::Mat validationImage = cv::imread(validationImagePath);
    cvtColor(validationImage, validationImage, CV_RGBA2GRAY);
    std::string fisheyeImagePath = szDirectoryName + "leftFisheye0000.png";
    cv::Mat fisheyeImage = cv::imread(fisheyeImagePath);
    cv::cvtColor(fisheyeImage, fisheyeImage, CV_RGBA2GRAY);
    cv::Mat xd = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1);
    cv::Mat yd = cv::Mat(CameraIntrinsics.height_px, CameraIntrinsics.width_px, CV_32FC1);
    distortionEquation(&xd, &yd);
    float max_x = 0.00;
    float max_y = 0.00;

    for (uint64_t i = 0; i < CameraIntrinsics.height_px; i++)
    {
        for (uint64_t j = 0; j < CameraIntrinsics.width_px; ++j)
        {
            xd.at<float>(i, j) = round(xd.at<float>(i, j) + spInputC->GetValue()[0]);
            yd.at<float>(i, j) = round(yd.at<float>(i, j) + spInputC->GetValue()[1]);
            if (xd.at<float>(i, j) > max_x)
            {
                max_x = xd.at<float>(i, j);
            }
            if (yd.at<float>(i, j) > max_y)
            {
                max_y = yd.at<float>(i, j);
            }
        }
    }

    cv::Mat mappedImage = cv::Mat(max_y + 1, max_x + 1, CV_8UC1);
    for (uint64_t i = 0; i < CameraIntrinsics.height_px; i++)
    {
        for (uint64_t j = 0; j < CameraIntrinsics.width_px; j++) {
            if (std::isnan(xd.at<float>(i, j)) || std::isnan(yd.at<float>(i, j)))
            {
                mappedImage.at<uchar>(i, j) = fisheyeImage.at<uchar>(i, j);
            }
            else
            {
                if (xd.at<float>(i, j) < 0) {
                    xd.at<float>(i, j) = 0.0;
                }
                if (yd.at<float>(i, j) < 0) {
                    yd.at<float>(i, j) = 0.0;
                }
                mappedImage.at<uchar>((int)yd.at<float>(i, j), (int)xd.at<float>(i, j)) = fisheyeImage.at<uchar>(i, j);
            }

        }
    }
    resize(validationImage, validationImage, mappedImage.size());
    cv::Mat differenceImage = mappedImage - validationImage;
}

void KB4Capturer::InitializeCapturer()
{
    FirstFisheye.TranslationParameters[0] = 0.0;
    FirstFisheye.TranslationParameters[1] = 0.0;
    FirstFisheye.TranslationParameters[2] = 0.0;
    FirstFisheye.spOrientation->SetElement(0, 0, 1.0);
    FirstFisheye.spOrientation->SetElement(0, 1, 0.0);
    FirstFisheye.spOrientation->SetElement(0, 2, 0.0);
    FirstFisheye.spOrientation->SetElement(1, 0, 0.0);
    FirstFisheye.spOrientation->SetElement(1, 1, 1.0);
    FirstFisheye.spOrientation->SetElement(1, 2, 0.0);
    FirstFisheye.spOrientation->SetElement(2, 0, 0.0);
    FirstFisheye.spOrientation->SetElement(2, 1, 0.0);
    FirstFisheye.spOrientation->SetElement(2, 2, 1.0);
    SecondFisheye.TranslationParameters[0] = 0.064;
    SecondFisheye.TranslationParameters[1] = 0.0;
    SecondFisheye.TranslationParameters[2] = 0.0;
    SecondFisheye.spOrientation->SetElement(0, 0, 1.0);
    SecondFisheye.spOrientation->SetElement(0, 1, 0.0);
    SecondFisheye.spOrientation->SetElement(0, 2, 0.0);
    SecondFisheye.spOrientation->SetElement(1, 0, 0.0);
    SecondFisheye.spOrientation->SetElement(1, 1, 1.0);
    SecondFisheye.spOrientation->SetElement(1, 2, 0.0);
    SecondFisheye.spOrientation->SetElement(2, 0, 0.0);
    SecondFisheye.spOrientation->SetElement(2, 1, 0.0);
    SecondFisheye.spOrientation->SetElement(2, 2, 1.0);
    DepthResolution.width = 640;
    DepthResolution.height = 480;
    CameraIntrinsics.width_px = 848;
    CameraIntrinsics.height_px = 800;
    InputFisheyeResolution.width = 1332;
    InputFisheyeResolution.height = 998;
    spColorFocalLength->SetValue(260, 260, 0);
    spDepthFocalLength->SetValue(260, 260, 0);
    ColorResolution.width = 640;
    ColorResolution.height = 480;
    spColorFOV->SetValue((2 * std::atan(ColorResolution.width / (2 * spColorFocalLength->GetValue()[0])) * 180 / M_PI), (2 * std::atan(ColorResolution.height / (2 * spColorFocalLength->GetValue()[1])) * 180 / M_PI));
    spDepthFOV->SetValue((2 * std::atan(DepthResolution.width / (2 * spDepthFocalLength->GetValue()[0])) * 180 / M_PI), (2 * std::atan(DepthResolution.height / (2 * spDepthFocalLength->GetValue()[1])) * 180 / M_PI));
    CameraIntrinsics.w = 0.9622222222222222;
    spInputC->SetValue(InputFisheyeResolution.width / 2, InputFisheyeResolution.height / 2);
    CameraIntrinsics.c_x_px = 423.2913f;
    CameraIntrinsics.c_y_px = 399.8463f;
    spInputFisheyeFocalLength->SetValue(286.7, 286.7);
    CameraIntrinsics.f_x_px = 285.5378f;
    CameraIntrinsics.f_y_px = 286.81f;
    spInputFisheyeFOV->SetValue((2 * std::atan(InputFisheyeResolution.width / (2 * spInputFisheyeFocalLength->GetValue()[0])) * 180 / M_PI), (2 * std::atan(InputFisheyeResolution.height / (2 * spInputFisheyeFocalLength->GetValue()[1])) * 180 / M_PI));
    CameraIntrinsics.k1 = -0.006801581f;
    CameraIntrinsics.k2 = 0.05125839f;
    CameraIntrinsics.k3 = -0.04885207f;
    CameraIntrinsics.k4 = 0.00964272f;

}


void FOVCapturer::InitializeCapturer()
{
    FirstFisheye.TranslationParameters[0] = 0.0;
    FirstFisheye.TranslationParameters[1] = 0.0;
    FirstFisheye.TranslationParameters[2] = 0.0;
    FirstFisheye.spOrientation->SetElement(0, 0, 1.0);
    FirstFisheye.spOrientation->SetElement(0, 1, 0.0);
    FirstFisheye.spOrientation->SetElement(0, 2, 0.0);
    FirstFisheye.spOrientation->SetElement(1, 0, 0.0);
    FirstFisheye.spOrientation->SetElement(1, 1, 1.0);
    FirstFisheye.spOrientation->SetElement(1, 2, 0.0);
    FirstFisheye.spOrientation->SetElement(2, 0, 0.0);
    FirstFisheye.spOrientation->SetElement(2, 1, 0.0);
    FirstFisheye.spOrientation->SetElement(2, 2, 1.0);
    SecondFisheye.TranslationParameters[0] = 0.064;
    SecondFisheye.TranslationParameters[1] = 0.0;
    SecondFisheye.TranslationParameters[2] = 0.0;
    SecondFisheye.spOrientation->SetElement(0, 0, 1.0);
    SecondFisheye.spOrientation->SetElement(0, 1, 0.0);
    SecondFisheye.spOrientation->SetElement(0, 2, 0.0);
    SecondFisheye.spOrientation->SetElement(1, 0, 0.0);
    SecondFisheye.spOrientation->SetElement(1, 1, 1.0);
    SecondFisheye.spOrientation->SetElement(1, 2, 0.0);
    SecondFisheye.spOrientation->SetElement(2, 0, 0.0);
    SecondFisheye.spOrientation->SetElement(2, 1, 0.0);
    SecondFisheye.spOrientation->SetElement(2, 2, 1.0);
    DepthResolution.width = 640;
    DepthResolution.height = 480;
    CameraIntrinsics.width_px = 640;
    CameraIntrinsics.height_px = 480;
    InputFisheyeResolution.width = 1332;
    InputFisheyeResolution.height = 998;
    spColorFocalLength->SetValue(260, 260, 0);
    spDepthFocalLength->SetValue(260, 260, 0);
    ColorResolution.width = 640;
    ColorResolution.height = 480;
    spColorFOV->SetValue((2 * std::atan(ColorResolution.width / (2 * spColorFocalLength->GetValue()[0])) * 180 / M_PI), (2 * std::atan(ColorResolution.height / (2 * spColorFocalLength->GetValue()[1])) * 180 / M_PI));
    spDepthFOV->SetValue((2 * std::atan(DepthResolution.width / (2 * spDepthFocalLength->GetValue()[0])) * 180 / M_PI), (2 * std::atan(DepthResolution.height / (2 * spDepthFocalLength->GetValue()[1])) * 180 / M_PI));
    CameraIntrinsics.w = 0.92;
    spInputC->SetValue(InputFisheyeResolution.width / 2, InputFisheyeResolution.height / 2);
    CameraIntrinsics.c_x_px = CameraIntrinsics.width_px / 2 - 0.5;
    CameraIntrinsics.c_y_px = CameraIntrinsics.height_px / 2 - 0.5;
    spInputFisheyeFocalLength->SetValue(260, 260);
    CameraIntrinsics.f_x_px = 260;
    CameraIntrinsics.f_y_px = 260;
    spInputFisheyeFOV->SetValue((2 * std::atan(InputFisheyeResolution.width / (2 * spInputFisheyeFocalLength->GetValue()[0])) * 180 / M_PI), (2 * std::atan(InputFisheyeResolution.height / (2 * spInputFisheyeFocalLength->GetValue()[1])) * 180 / M_PI));
}

void CreateCameraInterpolator(const char* const filename, double viewAngle, vtkSmartPointer<vtkCameraInterpolator>* const spCamerainterp, std::vector<double>* const pTimes)
{
    *spCamerainterp = vtkSmartPointer<vtkCameraInterpolator>::New();
    (*spCamerainterp)->SetInterpolationTypeToSpline();

    std::ifstream cameraFile(filename);
    std::string line;
    if (cameraFile)
    {
        while (getline(cameraFile, line))
        {
            const char* refBuf = line.c_str();

            double time;
            double params[14] = { 0 };
#ifdef _WIN32
            sscanf_s(refBuf, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf\n", &time,
                &params[0], &params[1], &params[2],
                &params[3], &params[4], &params[5],
                &params[6], &params[7], &params[8],
                &params[9], &params[10],&params[11],
                &params[12],&params[13]);
#else
            sscanf(refBuf, "%lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf %lf\n", &time,
                &params[0], &params[1], &params[2],
                &params[3], &params[4], &params[5],
                &params[6], &params[7], &params[8],
                &params[9], &params[10], &params[11],
                &params[12], &params[13]);
#endif
            vtkSmartPointer<vtkTransform> spTransform = vtkSmartPointer<vtkTransform>::New();
            vtkSmartPointer<vtkCamera> spCamera = vtkSmartPointer<vtkCamera>::New();
            spCamera->SetParallelProjection(0);
            spCamera->SetPosition(params[0], params[1], params[2]);
            spCamera->SetFocalPoint(params[3], params[4], params[5]);
            spCamera->SetViewUp(params[6], params[7], params[8]);
            spCamera->SetViewAngle(viewAngle);
            (*spCamerainterp)->AddCamera(time, spCamera);
            (*pTimes).push_back(time);
        }
    }
    else
    {
        fprintf(stderr, "\nerror - can not find file %s \n", filename);
        return;
    }
}

std::map<std::string, std::string> ParseCmdArgs(int argc, const char* const * argv)
{
    std::map<std::string, std::string> args;
    for (int i = 1; i < argc - 1; i += 2)
    {
        std::string userArg = std::string(argv[i]);
        std::transform(userArg.begin(), userArg.end(), userArg.begin(), ::tolower);
        args.insert(std::pair<std::string, std::string>(userArg, std::string(argv[i + 1])));
    }
    return args;
}

void controllerAnimationRead(const char* const filename, int* const pControllerAnimationSize, std::vector<std::vector<double>>* const pControllerAnim)
{
    *pControllerAnimationSize = 0.0;
    std::ifstream animationFile(filename);
    std::string line;
    if (animationFile)
    {
        std::vector<double> params(7);
        while (animationFile >> params[0] >> params[1] >> params[2] >> params[3] >> params[4] >> params[5] >> params[6])
        {
            (*pControllerAnim).push_back(params);
            (*pControllerAnimationSize)++;
        }
    }
}

CFakeImgCapturer::CFakeImgCapturer()
{
    spColorFocalLength = vtkSmartPointer<vtkCoordinate>::New();
    spDepthFocalLength = vtkSmartPointer<vtkCoordinate>::New();
    FirstFisheye.spOrientation = vtkSmartPointer<vtkMatrix3x3>::New();
    SecondFisheye.spOrientation = vtkSmartPointer<vtkMatrix3x3>::New();
    spColorFOV = vtkSmartPointer<vtkCoordinate>::New();
    spDepthFOV = vtkSmartPointer<vtkCoordinate>::New();
    spInputFisheyeFOV = vtkSmartPointer<vtkCoordinate>::New();
    spInputC = vtkSmartPointer<vtkCoordinate>::New();
    spInputFisheyeFocalLength = vtkSmartPointer<vtkCoordinate>::New();
}

void PrintUsage()
{
    cout << "Usage: \n\tsynthetic_generator\t --mesh name_of_the_mesh_file (Details: required argument. Allowed extensions are .obj or .ply)";
    cout << "       \n\t                   \t --animation name_of_the_camera_poses_file (Details: required for controller involved scenarios.)";
    cout << "       \n\t                   \t --calibrationfile name_of_the_calibration_file (Details: required argument.)";
    cout << "       \n\t                   \t --directory name_of_the_directory (Details: optional argument. Name of the directory which contains output data. Default name is ControllerData)";
    cout << "       \n\t                   \t --controlleranimation name_of_the_controller_poses_file (Details: optional argument.)";
    cout << "       \n\t                   \t --record (Details: optional argument. If specified poses are saved accordingly.)";
    cout << "       \n\t                   \t --texture name_of_the_texture_file (Details: optional argument.)";
    cout << "       \n\t                   \t --numberofframes value (Details: optional argument. Represents the number of interpolation points for the entire animation. Default is 100.)";
    cout << "       \n\t                   \t --renderingtimeinterval value (Details: optional argument. Represents the time in ms how often to render a frame. Default is 100ms.)";
    cout << "       \n\t                   \t --scenario name_of_the_scenario_to_run (Details: By default single controller. Other possible values: multiplecontrollers or interactive)";
    cout << "       \n\t                   \t --secondcontrolleranimation name_of_the_second_controller_poses_file (Details: optional argument. needs to be used in conjunction with --scenario multiplecontrollers )";
    cout << "       \n\t                   \t --controllerfile name_of_the_controller_mesh_model (Details: required argument for any controller scenario. Needs to be of .stl format)";
    cout << "       \n";
}

void CFakeImgCapturer::SetCameraIntrinsics(const rc_ExtendedCameraIntrinsics& camera)
{
    CameraIndex = camera.index;
    ImageFormat = camera.format;
}
