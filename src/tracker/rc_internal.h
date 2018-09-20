#ifndef rc_internal_h
#define rc_internal_h

#include "rc_tracker.h"

typedef struct { rc_Timestamp time_us; rc_SessionId session_id; } rc_SessionTimestamp;
typedef struct { rc_Pose pose_m; rc_SessionTimestamp time_destination; } rc_RelocEdge;
typedef struct { rc_SessionTimestamp time; } rc_MapNode;
typedef union { struct { uint32_t nodes, edges, features, unique_features, relocalization_bins; }; uint32_t items[5]; } rc_StorageStats;

RCTRACKER_API int rc_getRelocalizationEdges(rc_Tracker* tracker, rc_Timestamp *source, rc_RelocEdge **edges);
RCTRACKER_API int rc_getRelocalizationPoses(rc_Tracker* tracker, rc_Pose **poses);
RCTRACKER_API int rc_getMapNodes(rc_Tracker* tracker, rc_MapNode **mapnodes_timestamps);
RCTRACKER_API rc_StorageStats rc_getStorageStats(rc_Tracker* tracker);

inline bool operator< (const rc_SessionTimestamp& lhs, const rc_SessionTimestamp& rhs) {
    return lhs.session_id < rhs.session_id ||
            (lhs.session_id == rhs.session_id && lhs.time_us < rhs.time_us);
}

#endif // rc_internal_h
