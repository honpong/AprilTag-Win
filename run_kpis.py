#!/usr/bin/env python

import os
import sys
import subprocess, csv
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


zd_translation = []
zd_rotation = []
sn_translation = []
sn_rotation = []
for filename in enumerate_all_files_matching(kpi_dir+"/zero-drift","stereo.rc"):
    cpu_output = filename + result_extension
    kpi_output = filename + result_extension + ".result.csv"
    if not os.path.isfile(cpu_output):
        print "Warning: Skipping", filename
        continue
    print filename
    interval = filename + ".intervals.txt"
    command = ["./ThirdParty/tm2-validation-scripts/M1-staticnoise-standalone.py", cpu_output, interval, kpi_output]
    subprocess.call(command)
    with open(kpi_output, "rb") as csvfile:
        for row in csv.DictReader(csvfile, delimiter=","):
            if row[""] == "Zero Drift":
                zd_translation.append(float(row["Translation Dist Avg"]))
                zd_rotation.append(float(row["Rotation Max"]))
            elif row[""] == "Static Noise (avg of phases)":
                sn_translation.append(float(row["Translation Dist Avg"]))
                sn_rotation.append(float(row["Rotation Max"]))

tr_scale = []
tr_error = []
for filename in enumerate_all_files_matching(kpi_dir+"/vr-translation","stereo.rc"):
    cpu_output = filename + result_extension
    kpi_output = filename + result_extension + ".result.csv"
    if not os.path.isfile(cpu_output):
        print "Warning: Skipping", filename
        continue
    print filename
    interval = filename + ".intervals.txt"
    axes = filename + ".robotaxes.txt"
    robot = filename + ".robotposes.txt"
    command = ["./ThirdParty/tm2-validation-scripts/M2-VR_translation.py", cpu_output, interval, axes, robot, kpi_output]
    subprocess.call(command)
    with open(kpi_output, "rb") as csvfile:
        for row in csv.DictReader(csvfile, delimiter=","):
            tr_scale.append(float(row['Average Translation Scale Error'])*100)
            tr_error.append(float(row['Average Translation Error Ratio'])*100)

rotation_errors = []
for filename in enumerate_all_files_matching(kpi_dir+"/rotation","stereo.rc"):
    cpu_output = filename + result_extension
    kpi_output = filename + result_extension + ".result.csv"
    if not os.path.isfile(cpu_output):
        print "Warning: Skipping", filename
        continue
    print filename
    interval = filename + ".intervals.txt"
    robot = filename + ".robotposes.txt"
    command = ["./ThirdParty/tm2-validation-scripts/M3-rotation_metric.py", cpu_output, interval, robot, kpi_output]
    subprocess.call(command)
    with open(kpi_output, "rb") as csvfile:
        for row in csv.DictReader(csvfile, delimiter=","):
            rotation_errors.append(float(row['Rotation Error']))

def format_list(fmt_string, l):
    return " ".join([fmt_string % i for i in l])

print "Zero drift translations:", format_list("%.2f", zd_translation)
print "Zero drift rotations:", format_list("%.2f", zd_rotation)
print "Static noise translations:", format_list("%.2f", sn_translation)
print "Static noise rotations:", format_list("%.2f", sn_rotation)
print "Translation scales:", format_list("%.2f", tr_scale)
print "Translation error ratios:", format_list("%.2f", tr_error)
print "Rotation errors: ", format_list("%.2f", rotation_errors)

print ""
print "KPI Summary - mean (std)"
print "============================="
print "Static noise:"
print "  Translation (target < 1mm):  %0.2f (%0.3f)" % (np.mean(sn_translation), np.std(sn_translation))
print "  Rotation (target < 0.1deg):  %0.2f (%0.3f)" % (np.mean(sn_rotation), np.std(sn_rotation))
print "Zero drift:"
print "  Translation (target < 3mm):  %0.2f (%0.3f)" % (np.mean(zd_translation), np.std(zd_translation))
print "  Rotation (target < 0.5deg):  %0.2f (%0.3f)" % (np.mean(zd_rotation), np.std(zd_rotation))
print "VR Translation:"
print "  Scale (target < 5%%):         %0.2f (%0.3f)" % (np.mean(tr_scale), np.std(tr_scale))
print "  Error ratio (target < 1%%):   %0.2f (%0.3f)" % (np.mean(tr_error), np.std(tr_error))
print "Rotation test:"
print "  Max error (target < 1deg):   %0.2f (%0.3f)" % (np.mean(rotation_errors), np.std(rotation_errors))
