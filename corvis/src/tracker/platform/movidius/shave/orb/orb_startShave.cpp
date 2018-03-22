#include <mv_types.h>
#include <moviVectorTypes.h>
#include "tracker.h"
#include "orb_descriptor.h"
#include "orb_descriptor_shave.h"
#include <stdio.h>

extern "C"
__attribute__((dllexport))
void compute_descriptors(orb_data descriptors[], float2* xy, const tracker::image &image, const int num_orb)
{
    for (int i = 0; i < num_orb; ++i) {
        orb_shave orbs(xy[i][0], xy[i][1], image, &descriptors[i]);
    }
}
