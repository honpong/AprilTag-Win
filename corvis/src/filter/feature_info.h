#ifndef CORVIS_FEATURE_INFO
#define CORVIS_FEATURE_INFO

struct corvis_feature_info {
    uint64_t id;
    float x, y;
    float wx, wy, wz;
    float stdev;
};

#endif
