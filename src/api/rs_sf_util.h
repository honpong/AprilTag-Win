#include "rs_shapefit.h"
#include <Eigen/Dense>
#include <memory>

void set_to_zeros(rs_sf_image* img)
{
    memset(img->data, 0, img->num_char());
}