#!/usr/bin/env python

import sys
from measure import measure

sys.path.extend(['../'])

if len(sys.argv) != 2:
    print "Usage:", sys.argv[0], "<data_folder>"
    sys.exit(1)


replay_file = sys.argv[1]

measure(replay_file, "simulator", realtime=True, shell=True,
        vis=True, sim=True)
