#!/usr/bin/env python

import os
import sys
import subprocess
import numpy as np
from collections import defaultdict

if len(sys.argv) != 3:
    print "Usage:", sys.argv[0], "<kpi-dir> <result-extension>"
    sys.exit(1)

kpi_dir, result_extension = sys.argv[1], sys.argv[2]

def enumerate_all_files_matching(path, extension):
    for dirpath, dirnames, filenames in os.walk(path):
        dirnames.sort()
        filenames.sort()
        for filename in [f for f in filenames if f.endswith(extension)]:
            yield os.path.join(dirpath, filename)

radii = ["0.50", "0.75", "1.00"]
margin = ["0.075", "0.100", "0.150"]

kpis = defaultdict(dict)
for r in radii:
    # for the same radii, the number of total GT crossing events will be the same but the total number of TM2 crossings will be different
    for m in margin:
        total_result = None
        print "\nRadius %sm with margin %sm" % (r, m)
        for filename in enumerate_all_files_matching(kpi_dir,"stereo.rc.tum"):
            if "samer" in filename or "mapping" in filename:
                print "Excluding", filename, "based on name which implies non VR gameplay"
                continue
            gt = filename
            cpu_output = filename[:-4] + result_extension
            if not os.path.isfile(cpu_output) or not os.path.isfile(gt):
                print "Warning: Skipping", filename
                continue
            print filename
            command = "build/check_radius \"%s\" \"%s\" %s %s" % (gt, cpu_output, r, m)

            result_text = subprocess.check_output(command, stderr=subprocess.STDOUT, shell=True)
            for line in result_text.splitlines():
                if line.startswith("CSVContent"):
                    result = [float(i) for i in line.split(",")[1:]]
                    if total_result is None:
                        total_result = np.array(result)
                    else:
                        total_result = total_result + np.array(result)
                    print result
        kpis[r][m] = total_result

result_string  = "\nSummary: Safety KPI calculated on %s which had %.2fs of data\n" % (kpi_dir, kpis[radii[0]][margin[0]][0])
result_string += "radius\tmargin\tt outside\tGT crossings\tfalse negatives (%)\tTM2 crossings\tfalse positives (%)\n"
for r in radii:
    for m in margin:
        (total_time, time_outside, gt, tp, fn, tm2, tn, fp) = kpis[r][m]
        result_string += "%6s\t%6s\t%9.2f\t       %5d\t    %5d (%6.2f%%) \t        %5d\t    %5d (%6.2f%%)\n" % (r, m, time_outside, gt, fn, 100*fn/(gt+0.0001), tm2, fp, 100*fp/(tm2+0.0001))
print result_string,
