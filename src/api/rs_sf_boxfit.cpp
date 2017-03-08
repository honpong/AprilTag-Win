#include "rs_sf_boxfit.h"

rs_sf_boxfit::rs_sf_boxfit(const rs_sf_intrinsics * camera) : rs_sf_planefit(camera)
{
}

rs_sf_status rs_sf_boxfit::process_depth_image(const rs_sf_image * img)
{
    return rs_sf_status();
}

rs_sf_status rs_sf_boxfit::track_depth_image(const rs_sf_image * img)
{
    return rs_sf_status();
}

int rs_sf_boxfit::num_detected_boxes() const
{
    return 0;
}
