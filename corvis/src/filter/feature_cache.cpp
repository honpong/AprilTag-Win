#include "feature_cache.h"
#include "feature_descriptor.h"
#include "state_vision.h"

#define DISABLE_FEATURE_CACHE

using namespace std;

void feature_cache::add(state_vision_feature * feature)
{
#ifdef DISABLE_FEATURE_CACHE
    delete feature;
    return;
#endif
    feature->last_variance = feature->variance();

    if(feature->last_variance > state_vision_feature::initial_var/(2*2)) {
        delete feature;
        return;
    }

    features.push_back(feature);
    if(features.size() > max_size) {
        state_vision_feature * f = features.front();
        features.pop_front();
        delete f;
    }
}

state_vision_feature * feature_cache::query(const descriptor & d)
{
#ifdef DISABLE_FEATURE_CACHE
    return NULL;
#endif
    float match_thresh2 = 0.5*0.5;
    float best_dist2 = 2;
    state_vision_feature * match = NULL;

    for(state_vision_feature * f : features) {
        float dist2 = descriptor_dist2(f->descriptor, d);
        if(dist2 < best_dist2) {
            match = f;
            best_dist2 = dist2;
        }
    }
    
    if(best_dist2 < match_thresh2 && match) {
        //fprintf(stderr, "found %llu with dist %f, removing from cache, cache size %lu\n", match->id, best_dist2, features.size());
        match->recovered_score = best_dist2;
        features.remove(match);
        return match;
    }
    return NULL;
}

void feature_cache::clear()
{
    features.remove_if([&](state_vision_feature *f) {
        delete f;
        return true;
    });
}
