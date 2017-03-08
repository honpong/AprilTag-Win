#pragma once
#ifndef rs_sf_boxfit_h
#define rs_sf_boxfit_h

#include "rs_sf_planefit.h"

struct rs_sf_boxfit : public rs_sf_planefit
{
    rs_sf_boxfit(const rs_sf_intrinsics* camera);
    rs_sf_status process_depth_image(const rs_sf_image* img);
    rs_sf_status track_depth_image(const rs_sf_image* img);

    int num_detected_boxes() const;
};

#endif // ! rs_sf_boxfit_h