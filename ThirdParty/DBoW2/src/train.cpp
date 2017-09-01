#include <string>
#include <vector>
#include <windows.h>
#include "cv.hpp"
#include "highgui.h"
#include "TemplatedVocabulary.h"
#include "FORB.h"

void GetFileListUnderDir(std::string& strDir, std::vector<std::string>& vFileNames)
{
    const static int iSample = 6;
    int iFile = 0;
    std::vector<std::string> vFiles;

    std::string strDirWildCard = strDir + std::string("\\*");
    
    WIN32_FIND_DATA fd;
    HANDLE hFind = FindFirstFile(strDirWildCard.c_str(), &fd);
    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            // read all (real) files in current folder
            // , delete '!' read other 2 default folder . and ..
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            {   
                if (iFile%iSample == 0)
                {
                    std::string strPathName = strDir + std::string("\\") + std::string(fd.cFileName);
                    vFiles.push_back(strPathName);
                }
                ++iFile;
            }
        }
        while (FindNextFile(hFind, &fd));
        FindClose(hFind);
    }

    vFileNames = vFiles;
}

int main(int argc, char** argv)
{
    DBoW2::TemplatedVocabulary<DBoW2::FORB::TDescriptor, DBoW2::FORB> voc(10, 6, DBoW2::WeightingType::TF_IDF, DBoW2::ScoringType::L1_NORM);
    voc.loadFromTextFile(std::string("D:\\dataset\\voc.txt"));
    if (argc > 2)
    {
        std::vector<std::string> vFiles;
        std::string strDataset(argv[1]);
        std::string strVocFile(argv[2]);
        GetFileListUnderDir(strDataset, vFiles);

        std::cout << "Feature detection and extraction " << std::endl;
        cv::Mat matColor = cv::Mat::zeros(240, 320, CV_8UC3);
        cv::Mat matGray = cv::Mat::zeros(240, 320, CV_8UC1);
        cv::ORB orb(1000, 1.2f, 8, 31, 0, 2, cv::ORB::FAST_SCORE);
        
        std::vector<std::vector<DBoW2::FORB::TDescriptor>> trainingfeatures;
        trainingfeatures.reserve(1000 * 10000);
        for (unsigned int i = 0; i < vFiles.size(); ++i)
        {   
            matColor = cv::imread(vFiles[i]);
            cv::cvtColor(matColor, matGray, CV_RGB2GRAY);
            std::vector<cv::KeyPoint> vFeatures;
            cv::Mat matDesc;
            std::vector<DBoW2::FORB::TDescriptor> descPerImg;
            orb(matGray, cv::noArray(), vFeatures, matDesc);

            for (int i = 0; i < matDesc.rows; ++i)
            {
                descPerImg.push_back(matDesc.row(i).clone());
            }
            trainingfeatures.push_back(descPerImg);
        }
        std::cout << std::endl;

        std::cout << "training..." << std::endl;
        DBoW2::TemplatedVocabulary<DBoW2::FORB::TDescriptor, DBoW2::FORB> voc(10, 6, DBoW2::WeightingType::TF_IDF, DBoW2::ScoringType::L1_NORM);
        voc.create(trainingfeatures);
        std::cout << "saving vocabulary file..." << std::endl;
        voc.saveToTextFile(strVocFile.c_str());
    }

    return 0;
}