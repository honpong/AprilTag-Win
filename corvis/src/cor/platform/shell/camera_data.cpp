//
//  camera_data.cpp
//  RC3DK
//
//  Created by Eagle Jones on 3/17/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#include "../camera_data.h"

camera_data::camera_data(): image_handle(nullptr, nullptr), image(nullptr), timestamp(0), width(0), height(0), stride(0)
{
}

camera_data::camera_data(void *h): image_handle(h, [](void *h) {})
{
}

camera_data::~camera_data()
{
}
