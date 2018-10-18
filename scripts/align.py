#!/usr/bin/env python

from __future__ import print_function
import sys
import os, errno
from tumutil import write_tum, extract_csv_poses, extract_vive_csv_poses, offset_poses, apply_handeye, extract_pose_offset, read_handeye
import subprocess

if len(sys.argv) != 3:
    print("Usage:", sys.argv[0], "<input_folder> <output_folder>")
    sys.exit(1)

input_folder = sys.argv[1]
output_folder = sys.argv[2]

def ensure_path(path):
    try:
        os.makedirs(path)
    except OSError as exception:
        if exception.errno != errno.EEXIST:
            raise

def one_file_starting_with(path, prefix):
    files = [i for i in os.listdir(path) if os.path.isfile(os.path.join(path, i)) and i.startswith(prefix)]
    if len(files) == 0:
        print("Error: no files found starting with", prefix, "in", path)
        sys.exit(1)
    if len(files) > 1:
        print("Error: more than one file found ending with", prefix, "in", path)
        sys.exit(1)
    return os.path.join(path, files[0])

def one_file_ending_with(path, suffix):
    files = [i for i in os.listdir(path) if os.path.isfile(os.path.join(path, i)) and i.endswith(suffix)]
    if len(files) == 0:
        print("Error: no files found ending with", suffix, "in", path)
        sys.exit(1)
    if len(files) > 1:
        print("Error: more than one file found ending with", suffix, "in", path)
        sys.exit(1)
    return os.path.join(path, files[0])

def compare_poses(pose_gt_filename, pose_live_filename, aligned_output=False):
    if "COMPARE_POSES" in os.environ.keys():
        compare_path = os.environ["COMPARE_POSES"]
    else:
        compare_path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "../build/compare_poses")
    command = [compare_path, pose_gt_filename, pose_live_filename]
    if aligned_output: command.append("-a")
    result_text = subprocess.check_output(command, stderr=subprocess.STDOUT)
    for line in result_text.splitlines():
        if line.startswith(b"SpaceSeparated"):
            fields = line.split()
            return [float(f) for f in fields[1:]]
    print("Error: compare_poses failed, do you need to build it?")
    sys.exit(1)

def create_plots(pose_live_filename):
    for mode, extension in [["-x", "XYZ.pdf"],["-a", "RPY.pdf"]]:
        command = [sys.executable, os.path.join(os.path.dirname(os.path.realpath(__file__)),"tumview.py"), pose_live_filename + "_gt_ate.tum", pose_live_filename , mode, "-o", pose_live_filename + extension]
        subprocess.call(command)

ensure_path(output_folder)

hmd_gt_extension = "_raw_hmd.csv"
live_poses_file    = one_file_ending_with(input_folder, "_Pose_Loop_0.csv")
hmd_gt_poses_file  = one_file_ending_with(input_folder, hmd_gt_extension)
c1_gt_poses_file   = one_file_ending_with(input_folder, "_raw_controller1.csv")
c2_gt_poses_file   = one_file_ending_with(input_folder, "_raw_controller2.csv")

pose_basename = os.path.join(output_folder, os.path.basename(hmd_gt_poses_file)[:-len(hmd_gt_extension)])
print("Writing poses to %s*" % (pose_basename))

hmd_gt_raw_poses = extract_vive_csv_poses(hmd_gt_poses_file, 0)
c1_gt_raw_poses  = extract_vive_csv_poses(c1_gt_poses_file, 1)
c2_gt_raw_poses  = extract_vive_csv_poses(c2_gt_poses_file, 2)

off1 = extract_pose_offset(hmd_gt_poses_file, live_poses_file, 0)
off2 = extract_pose_offset(c1_gt_poses_file, live_poses_file, 1)
off3 = extract_pose_offset(c2_gt_poses_file, live_poses_file, 2)
offset = (off1 + off2 + off3)/3

write_tum(pose_basename + ".hmd.live.tum", extract_csv_poses(live_poses_file, 0))
write_tum(pose_basename + ".c1.live.tum", extract_csv_poses(live_poses_file, 1))
write_tum(pose_basename + ".c2.live.tum", extract_csv_poses(live_poses_file, 2))

hmd_gt_raw_poses = offset_poses(hmd_gt_raw_poses, offset)
c1_gt_raw_poses  = offset_poses(c1_gt_raw_poses, offset)
c2_gt_raw_poses  = offset_poses(c2_gt_raw_poses, offset)

hec_hmd         = read_handeye(one_file_starting_with(input_folder, "hec_hmd"))
hec_controller1 = read_handeye(one_file_starting_with(input_folder, "hec_controller1"))
hec_controller2 = read_handeye(one_file_starting_with(input_folder, "hec_controller2"))

hmd_gt_poses = apply_handeye(hmd_gt_raw_poses, hec_hmd)
c1_gt_poses  = apply_handeye(c1_gt_raw_poses, hec_controller1)
c2_gt_poses  = apply_handeye(c2_gt_raw_poses, hec_controller2)

write_tum(pose_basename + ".hmd.gt.tum", hmd_gt_poses)
write_tum(pose_basename + ".c1.gt.tum", c1_gt_poses)
write_tum(pose_basename + ".c2.gt.tum", c2_gt_poses)

print(" " * len("Result " + pose_basename + ".hmd" + ".gt.tum"), "# cmp, # live, ATE rmse, ATE max, RPE rmse, RPE max")

pose_results = compare_poses(pose_basename + ".hmd.gt.tum", pose_basename + ".hmd.live.tum", aligned_output=True)
print("Result", pose_basename + ".hmd.gt.tum", " ".join([str(f) for f in pose_results]))
create_plots(pose_basename + ".hmd.live.tum")

for extension in [".c1", ".c2"]:
    if extension == ".c1": swapped = ".c2"
    else:                  swapped = ".c1"
    result         = compare_poses(pose_basename + extension + ".gt.tum", pose_basename + extension + ".live.tum", aligned_output=True)
    swapped_result = compare_poses(pose_basename + swapped   + ".gt.tum", pose_basename + extension + ".live.tum")
    if swapped_result[4] < result[4]:
        print("Warning: best RPE alignment found when comparing live", extension, "to switched GT")
        result = compare_poses(pose_basename + swapped + ".gt.tum", pose_basename + extension + ".live.tum", aligned_output=True)
    print("Result", pose_basename + extension + ".gt.tum", " ".join([str(f) for f in result]))
    create_plots(pose_basename + extension + ".live.tum")