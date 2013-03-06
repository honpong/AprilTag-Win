# Created by Eagle Jones
# Copyright (c) 2012. RealityCap, Inc.
# All Rights Reserved.

track = tracker.tracker()
track.spacing = 7
track.levels = 5
track.thresh = 0.1
track.iter = 25
track.eps = .3
track.blocksize = 5
track.harris = False
track.harrisk = .04
track.sink = trackdata
track.groupsize = 32
track.maxgroupsize = 40
track.maxfeats = 120
track.max_tracking_error = 1.0
tracker.init(track)
