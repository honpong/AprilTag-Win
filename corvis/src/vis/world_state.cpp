#include "world_state.h"

void world_state::observe_feature(uint64_t timestamp, uint64_t feature_id, float x, float y, float z, bool good)
{
    Feature f;
    f.x = x;
    f.y = y;
    f.z = z;
    f.last_seen = timestamp;
    f.good = good;
    display_lock.lock();
    if(timestamp > current_feature_timestamp)
        current_feature_timestamp = timestamp;
    if(timestamp > current_timestamp)
        current_timestamp = timestamp;
    features[feature_id] = f;
    display_lock.unlock();
}

void world_state::observe_position(uint64_t timestamp, float x, float y, float z, float qw, float qx, float qy, float qz)
{
    Position p;
    p.timestamp = timestamp;
    quaternion q(qw, qx, qy, qz);
    p.g = transformation(q, v4(x, y, z, 0));
    display_lock.lock();
    path.push_back(p);
    if(timestamp > current_timestamp)
        current_timestamp = timestamp;
    display_lock.unlock();
}
