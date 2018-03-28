#!/usr/bin/env python

import os
import sys
import subprocess
import numpy as np

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

radii = ["0.5", "0.75", "1"]
margin = ["0.075", "0.10", "0.15"]

for r in radii:
    for m in margin:
        total_result = None
        result = "result_%sm_%sm.txt" % (r, m)
        print result
        res_file = open(result, "w")
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
            res_file.write(result_text)
        result_string  = "Summary:\n";
        result_string += "%.2fs of data, with %.2fs outside %sm + %sm\n" % (total_result[0], total_result[1], r, m)
        result_string += "%d GT crossings of %sm + %sm, %d had TM2 > %sm (%.0f%%)\n" % (total_result[2], r, m, total_result[3], r, total_result[3]*100/(total_result[2]+0.0001))
        result_string += "%d TM2 crossings of %sm, %d had GT > %sm - %sm (%.0f%%)\n" % (total_result[5], r, total_result[6], r, m, total_result[6]*100/(total_result[5]+0.0001))
        print result_string
        res_file.write(result_string)
