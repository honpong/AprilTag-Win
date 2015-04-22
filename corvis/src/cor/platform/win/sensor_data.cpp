//
//  sensor_data.cpp
//  RC3DK
//
//  Created by Eagle Jones on 3/17/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "../sensor_data.h"

camera_data::camera_data(void *h): image_handle(h, [](void *h) {})
{
}

camera_data::~camera_data()
{
}
