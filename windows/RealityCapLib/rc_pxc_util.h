//
//  rc_pxc_util.h
//  RCTracker
//
//  Created by Eagle Jones on 6/2/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#pragma once

#include "rc_intel_interface.h"
#include "libpxcimu_internal.h"
#include "pxcsensemanager.h"

struct RCSavedImage
{
    RCSavedImage(PXCImage *i, PXCImage::PixelFormat format) : image(i)
    {
        pxcStatus result = image->AcquireAccess(PXCImage::ACCESS_READ, format, &data);
        if (result != PXC_STATUS_NO_ERROR || !data.planes[0]) throw std::runtime_error("PXCImage->AcquireAccess failed!");
        image->AddRef();
    }

    ~RCSavedImage()
    {
        image->ReleaseAccess(&data);
        image->Release();
    }

    static void releaseOpaquePointer(void *h)
    {
        delete (RCSavedImage *)h;
    }

    PXCImage *image;
    PXCImage::ImageData data;
};

static rc_Vector rc_convertAcceleration(const imu_sample_t *s)
{
    rc_Vector acceleration_m__s2;
    //windows gives acceleration in g-units, so multiply by standard gravity in m/s^2
    //Also transform axes to be inline with RC definition
    acceleration_m__s2.x = -s->data[1] * 9.80665f;
    acceleration_m__s2.y = s->data[0] * 9.80665f;
    acceleration_m__s2.z = -s->data[2] * 9.80665f;
    return acceleration_m__s2;
}

static rc_Vector rc_convertAngularVelocity(const imu_sample_t *s)
{
    rc_Vector angular_velocity_rad__s;
    //windows gives angular velocity in degrees per second
    //Axes are different than accelerometer axes, and happen to be same as RC
    angular_velocity_rad__s.x = (float)(s->data[1] * M_PI / 180.f);
    angular_velocity_rad__s.y = -(float)(s->data[0] * M_PI / 180.f);
    angular_velocity_rad__s.z = (float)(s->data[2] * M_PI / 180.f);
    return angular_velocity_rad__s;
}
