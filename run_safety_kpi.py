#!/usr/bin/env python

import os
import sys
import subprocess
import numpy as np
from multiprocessing import Pool
from collections import defaultdict

if len(sys.argv) < 3:
    print "Usage:", sys.argv[0], "<kpi-dir> [<kpi-dir2> ...] <result-extension>"
    sys.exit(1)

kpi_dirs = sys.argv[1:-1]
result_extension = sys.argv[-1]
print kpi_dirs
print result_extension

def enumerate_all_files_matching(path, extension):
    for dirpath, dirnames, filenames in os.walk(path):
        dirnames.sort()
        filenames.sort()
        for filename in [f for f in filenames if f.endswith(extension)]:
            yield os.path.join(dirpath, filename)

def one_kpi(args_tuple):
    (gt, cpu_output, r, m) = args_tuple

    result = None
    try:
        command = ["build/check_radius", gt, cpu_output, r, m]
        result_text = subprocess.check_output(command, stderr=subprocess.STDOUT)
        for line in result_text.splitlines():
            if line.startswith("CSVContent"):
                result = np.array([float(i) for i in line.split(",")[1:]])
                break

    except subprocess.CalledProcessError as e:
        print "Error:", e.cmd, "returned code", e.returncode
        print "Output was:", e.output
    return (result, args_tuple)

radii = ["0.50", "0.75", "1.00"]
margin = ["0.075", "0.100", "0.150"]

kpis = defaultdict(dict)
worker_pool = Pool()
for r in radii:
    # for the same radii, the number of total GT crossing events will be the same but the total number of TM2 crossings will be different
    for m in margin:
        total_result = None
        print "\nRadius %sm with margin %sm" % (r, m)

        args = list()
        for kpi_dir in kpi_dirs:
            for filename in enumerate_all_files_matching(kpi_dir,"stereo.rc.tum"):
                if "samer" in filename or "mapping" in filename:
                    print "Excluding", filename, "based on name which implies non VR gameplay"
                    continue
                gt = filename
                cpu_output = filename[:-4] + result_extension
                if not os.path.isfile(cpu_output) or not os.path.isfile(gt):
                    print "Warning: Skipping", filename
                    continue
                args.append((gt, cpu_output, r, m))

        #for arg in args:
        #    result = one_kpi(arg)
        print "Processing", len(args), "files for radius", r, "margin", m
        for result_tuple in worker_pool.map(one_kpi, args):
            (result, arg) = result_tuple
            if result is None:
                print "Error: Problem with", arg
                continue
            print "Result for", arg[1]
            print result
            if total_result is None:
                total_result = result
            else:
                total_result += result

        kpis[r][m] = total_result

result_string  = "\nSummary: Safety KPI calculated on %s which had %.2fs of data\n" % (kpi_dirs, kpis[radii[0]][margin[0]][0])
result_string += "radius\tmargin\tt outside\tGT crossings\tfalse negatives (%)\tTM2 crossings\tfalse positives (%)\n"
for r in radii:
    for m in margin:
        (total_time, time_outside, gt, tp, fn, tm2, tn, fp) = kpis[r][m]
        result_string += "%6s\t%6s\t%9.2f\t       %5d\t    %5d (%6.2f%%) \t        %5d\t    %5d (%6.2f%%)\n" % (r, m, time_outside, gt, fn, 100*fn/(gt+0.0001), tm2, fp, 100*fp/(tm2+0.0001))
print result_string,
