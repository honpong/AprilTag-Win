# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

track = filter.tracker()
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
filter.init(track)
