import sys, os
import re
from collections import defaultdict
from numpy import *

import measure

if len(sys.argv) != 2:
    print "Usage:", sys.argv[0], "<sequence folder>"
    sys.exit(1)

folder_name = sys.argv[1]
configurations = defaultdict(list)
for dirname, dirnames, filenames in os.walk(folder_name):
    for filename in filenames:
        (ignore, config_name) = os.path.split(dirname)

        length_match = re.search("_L([\d.]+)", filename)
        distance_match = re.search("_PL([\d.]+)", filename)
        measurement = None
        distance = None
        if length_match: measurement = float(length_match.group(1))
        if distance_match: distance = float(distance_match.group(1))
        if not length_match and not distance_match:
            print "Malformed data filename:", filename, "skipping"
            continue

        test_case = {"path" : os.path.join(dirname, filename), 
                     "measurement" : measurement,
                     "distance" : distance}
        configurations[config_name].append(test_case)

for config_name in configurations:
    for test_case in configurations[config_name]:
        state = measure.measure(test_case["path"], config_name)
        test_case["result"] = state

for config_name in configurations:
    for test_case in configurations[config_name]:
        print "Test case:", config_name, test_case["path"]
        print "Total path length (m):", test_case["result"].total_distance
        print "Straight line length (m):", sqrt(sum(test_case["result"].T.v**2))
