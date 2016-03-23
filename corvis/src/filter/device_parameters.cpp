//
//  device_parameters.c
//  RC3DK
//
//  Created by Eagle Jones on 4/10/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//

#include "calibration_defaults.h"

void device_set_resolution(device_parameters *dc, int image_width, int image_height)
{
    auto &in = dc->color.intrinsics;
    int max_old_dim = in.width_px > in.height_px ? in.width_px : in.height_px;
    int max_new_dim = image_width > image_height ? image_width : image_height;

    in.width_px = image_width;
    in.height_px = image_height;
    in.c_x_px = (in.width_px - 1)/2.f;
    in.c_x_px = (in.height_px - 1)/2.f;
    // Scale the focal length depending on the resolution
    in.f_x_px = in.f_x_px * max_new_dim / max_old_dim;
    in.f_y_px = in.f_x_px;
}

// TODO: should this go away?
bool get_parameters_for_device(corvis_device_type type, device_parameters *dc)
{
    return calibration_load_defaults(type, *dc);
}
