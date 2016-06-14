//
//  camera_control_interface_dummy.cpp
//  RC3DK
//
//  Created by Eagle Jones on 12/11/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#include "camera_control_interface.h"

camera_control_interface::camera_control_interface(): platform_ptr(NULL)
{
    platform_ptr = NULL;
}

camera_control_interface::~camera_control_interface()
{
}

void camera_control_interface::init(void *platform_specific_object)
{
}

void camera_control_interface::release_platform_specific_object()
{
}

void camera_control_interface::focus_lock_at_current_position(std::function<void (uint64_t, float)> callback)
{
    callback(0, 1.);
}

void camera_control_interface::focus_lock_at_position(float position, std::function<void (uint64_t, float)> callback)
{
    callback(0, 1.);
}

void camera_control_interface::focus_once_and_lock(std::function<void (uint64_t, float)> callback)
{
    callback(0, 1.);
}

void camera_control_interface::focus_unlock()
{
}
