#include "calibration_data_store.h"
#include "calibration_json_store.h"

using namespace RealityCap;

unique_ptr<calibration_data_store> calibration_data_store::GetStore()
{
    return make_unique<calibration_json_store>();
}