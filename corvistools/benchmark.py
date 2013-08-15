#!/opt/local/bin/python2.7
import sys, os
import re
from collections import defaultdict

if len(sys.argv) != 2:
    print "Usage:", sys.argv[0], "<sequence folder>"
    sys.exit(1)

class TestCase():
    error = 0
    error_percent = 0

    def __init__(self, length, name = ""):
        self.length = length
        self.name = ""

    def add_result(self, length):
        self.result = length
        self.error = self.result - self.length
        if self.length == 0:
            self.error_percent = 100*self.error
        else:
            self.error_percent = 100*self.error / self.length
    
    def __str__(self):
        return "%.2f - %.2f = %.2f (%.1f%%)" % (self.result, self.length,
            self.error, self.error_percent)

    def __repr__(self):
        return self.__str__()

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
            L = TestCase(float(L_match.group(1)))
        if PL_match: 
            PL = TestCase(float(PL_match.group(1)))
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


# Run each test and save the result
for config_name in configurations:
    for test_case in configurations[config_name]:
        print "Running", test_case["path"]
        (L, PL) = run_measurement(test_case["path"], config_name)
        
        # Convert lengths to cm
        if test_case["L"]: test_case["L"].add_result(L)
        if test_case["PL"]: test_case["PL"].add_result(PL)


# Gather the results into a more presentable form
for config_name in configurations:
    for test_case in configurations[config_name]:
        print config_name, test_case["path"]
        if test_case["PL"]: print "PL\t%s" %  test_case["PL"]
        if test_case["L"]: print "L\t%s" % test_case["L"]

for config_name in configurations:
    lengths = filter(lambda tc: tc["L"], configurations[config_name])
    path_lengths = filter(lambda tc: tc["PL"], configurations[config_name])

    def failed_filter(tc, mname):
        return tc[mname].error > 5 and tc[mname].error_percent > 5

    failed_lengths = filter(lambda tc: failed_filter(tc, "L"),
        lengths)
    failed_lengths = sorted(failed_lengths, reverse=True,
        key=lambda tc: abs(tc["L"].error))

    failed_path_lengths = filter(lambda tc: failed_filter(tc, "PL"),
        path_lengths)
    failed_path_lengths = sorted(failed_path_lengths, reverse=True,
        key=lambda tc: abs(tc["PL"].error))

    print "\n", config_name, "summary"
    print "%d / %d L measurements failed" % (len(failed_lengths), len(lengths))
    print "%d / %d PL measurements failed" % (len(failed_path_lengths), len(path_lengths))
    print "Worst failed L measurement:",
    if len(failed_lengths) > 0:
        print failed_lengths[0]["path"], failed_lengths[0]["L"]
    else:
        print "All passed (yay!)"

    print "Worst failed PL measurement:",
    if len(failed_path_lengths) > 0:
        print failed_path_lengths[0]["path"], failed_path_lengths[0]["PL"]
    else:
        print "All passed (yay!)"
