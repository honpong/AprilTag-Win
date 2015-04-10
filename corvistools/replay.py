#!/usr/bin/env python
# Copyright (c) 2008-2012, Eagle Jones
# All rights reserved.
#
# This file is a part of the corvis framework, and is made available
# under the BSD license; please see LICENSE file for full text

import sys
from measure import measure

sys.path.extend(['../'])

if len(sys.argv) != 3:
    print "Usage:", sys.argv[0], "<data_filename> <configuration_name>"
    sys.exit(1)


replay_file = sys.argv[1]
configuration_name = sys.argv[2]

measure(replay_file, configuration_name, realtime=True, shell=True,
        vis=True)
