#!/opt/local/bin/python2.7
import sys, os
import re
from collections import defaultdict

if len(sys.argv) != 2:
    print "Usage:", sys.argv[0], "<sequence folder>"
    sys.exit(1)

# L is the measurement length
# PL is the total path length

# Set up the tests by scanning the sequences folder
folder_name = sys.argv[1]
configurations = defaultdict(list)
for dirname, dirnames, filenames in os.walk(folder_name, followlinks=True):
    for filename in filenames:
        (ignore, config_name) = os.path.split(dirname)

        L_match = re.search("_L([\d.]+)", filename)
        PL_match = re.search("_PL([\d.]+)", filename)
        L = None
        PL = None
        if L_match: 
            L = float(L_match.group(1))
        if PL_match: 
            PL = float(PL_match.group(1))
        if not L_match and not PL_match:
            print "Malformed data filename:", filename, "skipping"
            continue
        
        test_case = {"path" : os.path.join(dirname, filename), "L" : L, "PL" : PL}
        configurations[config_name].append(test_case)

import subprocess
from util.parse_tools import parse_stderr, parse_measure_stdout
def run_measurement(path, config_name):
    command = "measure.py %s %s" % (path, config_name)
    proc = subprocess.Popen([sys.executable, "measure.py", path, config_name], 
        bufsize=0, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout, stderr) = proc.communicate()
    retcode = proc.poll()
    if retcode:
        sys.stderr.write("==== ERROR: measure.py returned non-zero %d\n" % proc.returncode)
        sys.exit(retcode)

    parsed = parse_measure_stdout(stdout)
    L = None
    PL = None
    if "L" in parsed: L = float(parsed["L"]["data"][0][0])
    if "PL" in parsed: PL = float(parsed["PL"]["data"][0][0])
    if len(parsed["unconsumed"]):
        sys.stdout.write("\n".join(parsed["unconsumed"]))
        sys.stdout.write("\n")


    parsed = parse_stderr(stderr)
    if len(parsed["unconsumed"]):
        sys.stderr.write("\n".join(parsed["unconsumed"]))
        sys.stderr.write("\n")
    for key in parsed:
        if key == "unconsumed": continue
        if parsed[key]["count"] == 1 and parsed[key].has_key("data"):
            sys.stderr.write("Warning: %s (%d times): "  % (key, parsed[key]["count"]))
            for i in range(len(parsed[key]["data"][0])):
                sys.stderr.write("%s "  % parsed[key]["data"][0][i])
            sys.stderr.write("\n")
        else:
            sys.stderr.write("Warning: %s (%d times)\n" % (key, parsed[key]["count"]))

    return (L, PL)


def measurement_error(L, L_measured):
    err = abs(L_measured - L)
    if L == 0:
        return (err, err)
    return (err, 100.*err / L)

def measurement_string(L, L_measured):
    error, error_percent = measurement_error(L, L_measured)
    return "%.2fcm actual, %.2fcm measured, %.2fcm error (%.2f%%)" % (
        L, L_measured, error, error_percent)

L_errors_percent = []
PL_errors_percent = []
# Run each test as a subprocess and report the error
for config_name in configurations:
    for test_case in configurations[config_name]:
        print "Running", test_case["path"]
        L_error, L_error_percent = 0, 0
        PL_error, PL_error_percent = 0, 0
        (L, PL) = run_measurement(test_case["path"], config_name)

        has_L = test_case["L"] is not None
        has_PL = test_case["PL"] is not None
        # Length measurement
        if has_L:
            (L_error, L_error_percent) = measurement_error(test_case["L"], L)
            print "L\t%s" % measurement_string(test_case["L"], L)
            if test_case["L"] > 5:
                L_errors_percent.append(L_error_percent)
        # Path length measurement
        if has_PL:
            (PL_error, PL_error_percent) = measurement_error(test_case["PL"], PL)
            print "PL\t%s" % measurement_string(test_case["PL"], PL)
            PL_errors_percent.append(PL_error_percent)
        # Loop measurement (a path length measurement which returns to
        # the start)
        if has_L and has_PL and test_case["L"] <= 5:
            print "\t", "Loop error (L_measured / PL): %.2f%%" % (100.*L/test_case["PL"])


import numpy
def error_histogram(errors):
    bins = [0, 3, 10, 25, 50, 100]
    if len(errors) > 0 and max(errors) > 100:
        bins.append(max(errors))
    (counts, bins) = numpy.histogram(errors, bins)
    return (counts, bins)

def error_histogram_string(counts, bins):
    hist_str = ""
    bins_str = []
    for b in bins:
        bins_str.append("%.1f%%" % b)
    bins_str = bins_str[:-1]
    bins_str[-1] += "+"
    hist_str += "\t".join(bins_str) + "\n"
    counts = [str(count) for count in counts]
    hist_str += "\t".join(counts) + "\n"
    return hist_str

(counts, bins) = error_histogram(L_errors_percent)
print "Length error histogram (%d sequences)" % len(L_errors_percent)
print error_histogram_string(counts, bins)

(counts, bins) = error_histogram(PL_errors_percent)
print "Path length error histogram (%d sequences)" % len(PL_errors_percent)
print error_histogram_string(counts, bins)
