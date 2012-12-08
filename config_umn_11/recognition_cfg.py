# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

sys.path.extend(["recognition/", "recognition/.libs"])
import recognition

sift = recognition.recognition()
sift.octaves = 4
sift.levels = 3
sift.maxkeys = 10000
sift.dict_size = 30
sift.dict_file = "ww_clusters.txt"
sift.sink = siftdata
sift.maximum_feature_std_dev_percent = .05
recognition.recognition_init(sift, cal.out_width, cal.out_height)
