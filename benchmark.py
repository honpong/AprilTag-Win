#!/opt/local/bin/python2.7
import sys, os
import re
from collections import defaultdict
from numpy import *

import measure

if len(sys.argv) != 2:
    print "Usage:", sys.argv[0], "<sequence folder>"
    sys.exit(1)


class TestCase():
    error = 0
    error_percent = 0

    def __init__(self, length):
        self.length = length

    def add_result(self, length):
        self.result = length
        self.error = self.result - self.length
        if self.length == 0:
            self.error_percent = 100*self.error
        else:
            self.error_percent = 100*self.error / self.length
    
    def __str__(self):
        return "%.2f - %.2f = %.2f (%.1f%%)" % (self.length, self.result,
            self.error, self.error_percent)

# Set up the tests by scanning the sequences folder
folder_name = sys.argv[1]
configurations = defaultdict(list)
for dirname, dirnames, filenames in os.walk(folder_name):
    for filename in filenames:
        (ignore, config_name) = os.path.split(dirname)

        length_match = re.search("_L([\d.]+)", filename)
        distance_match = re.search("_PL([\d.]+)", filename)
        measurement = None
        distance = None
        if length_match: 
            measurement = TestCase(float(length_match.group(1)))
        if distance_match: 
            distance = TestCase(float(distance_match.group(1)))
        if not length_match and not distance_match:
            print "Malformed data filename:", filename, "skipping"
            continue

        test_case = {"path" : os.path.join(dirname, filename), 
                     "measurement" : measurement,
                     "distance" : distance}
        configurations[config_name].append(test_case)

# Run each test and save the result
for config_name in configurations:
    for test_case in configurations[config_name]:
        state = measure.measure(test_case["path"], config_name)
        if test_case["measurement"]:
            # Convert measurements to cm
            test_case["measurement"].add_result(100*float(sqrt(sum(state.T.v**2))))
        if test_case["distance"]:
            # Convert measurements to cm
            test_case["distance"].add_result(100*state.total_distance)

# Gather the results into a more presentable form
for config_name in configurations:
    for test_case in configurations[config_name]:
        print config_name, test_case["path"]
        if test_case["distance"]: print "PL\t%s" %  test_case["distance"]
        if test_case["measurement"]: print "L\t%s" % test_case["measurement"]
