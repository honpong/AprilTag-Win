//
//  rs_sf_demo_t265.hpp
//  shapefit
//
//  Created by Hon Pong (Gary) Ho on 2/28/19.
//

#ifndef rs_sf_demo_t265_h
#define rs_sf_demo_t265_h

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <opencv2/opencv.hpp>

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <iostream>
#include <map>
#include <chrono>
#include <mutex>
#include <thread>
#include <future>

#include "json/json.h"

struct rgb_cam
{
    virtual bool isOpened() { return false; }
    virtual bool get_image(cv::Mat& dst) = 0;
    virtual std::string name() { return "cam id unknown"; }
    virtual ~rgb_cam() {}
    static std::unique_ptr<rgb_cam> create();
};

#endif /* rs_sf_demo_t265_h */
