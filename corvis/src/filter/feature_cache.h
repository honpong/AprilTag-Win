#ifndef __FEATURE_CACHE_H
#define __FEATURE_CACHE_H

#include <list>
#include "feature_descriptor.h"

class state_vision_feature;

class feature_cache {
    public:
        feature_cache(int size = 200) : max_size(size) {};
        ~feature_cache() { clear(); };

        void add(state_vision_feature * feature);
        state_vision_feature * query(const descriptor & d);
        void clear();

    private:
        int max_size;
        std::list<state_vision_feature *> features;
};

#endif
