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
    int max_old_dim = dc->image_width > dc->image_height ? dc->image_width : dc->image_height;
    int max_new_dim = image_width > image_height ? image_width : image_height;

    dc->image_width = image_width;
    dc->image_height = image_height;
    dc->Cx = (dc->image_width - 1)/2.f;
    dc->Cy = (dc->image_height - 1)/2.f;
    // Scale the focal length depending on the resolution
    dc->Fx = dc->Fx * max_new_dim / max_old_dim;
    dc->Fy = dc->Fx;
}

// TODO: should this go away?
bool get_parameters_for_device(corvis_device_type type, device_parameters *dc)
{
    return calibration_load_defaults(type, *dc);
}
