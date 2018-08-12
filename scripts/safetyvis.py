#!/usr/bin/env python
from __future__ import print_function
import sys
import matplotlib
# Force matplotlib to not use any Xwindows backend.
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from tumview import read_tum
from bisect import bisect

if len(sys.argv) != 7:
    print("Usage:", sys.argv[0], "<centered_gt.tum> <centered_tm2.tum> <intervals.txt> <radius_m> <margin_m> <output.pdf>")
    sys.exit(1)

def read_intervals(filename):
    with open(filename) as f:
        intervals = []
        for line in f:
            intervals.append([int(i) for i in line.split()])
        return intervals

[gt_name, gt] = read_tum(sys.argv[1])
[tm2_name, tm2] = read_tum(sys.argv[2])
intervals = read_intervals(sys.argv[3])
highlights = []
for i in intervals:
    if i[2] == 0 and i[3] == 0: # gt_type and bad
        highlights.append([float(i[0])/1e6, float(i[1])/1e6])
print(highlights)
radius = float(sys.argv[4])
margin = float(sys.argv[5])
output_filename = sys.argv[6]

if 0 not in gt or 0 not in tm2:
    print("Missing device id 0 in GT or TM2")
    sys.exit(1)

fig, ax = plt.subplots()
TIME = 0
X = 1
Y = 2
tm2_handle, = ax.plot(tm2[0][X], tm2[0][Y], color='b', linewidth=0.5, alpha=0.4)
gt_handle,  = ax.plot(gt [0][X], gt [0][Y], color='g', linewidth=0.5, alpha=0.4)
for highlight in highlights:
    start = bisect(tm2[0][TIME], highlight[0])
    stop  = bisect(tm2[0][TIME], highlight[1])
    ax.plot(tm2[0][X][start:stop], tm2[0][Y][start:stop], color='b', linewidth=2)
    ax.plot(tm2[0][X][start], tm2[0][Y][start], color='r', marker='D')
    start = bisect(gt[0][TIME], highlight[0])
    stop  = bisect(gt[0][TIME], highlight[1])
    ax.plot(gt[0][X][start:stop], gt[0][Y][start:stop], color='g', linewidth=2)
    ax.plot(gt[0][X][start], gt[0][Y][start], color='r', marker='D')

ax.set_aspect('equal', 'datalim')
ax.legend((tm2_handle, gt_handle), ('tm2', 'gt'))
circle_radius = plt.Circle((0, 0), radius, color='r', fill=False, linewidth=1)
circle_margin = plt.Circle((0, 0), radius - margin, color='r', fill=False, linewidth=1)
circle_margin2 = plt.Circle((0, 0), radius - margin*2, color='r', fill=False, linewidth=1)

ax.add_artist(circle_radius)
ax.add_artist(circle_margin)
ax.add_artist(circle_margin2)

fig.savefig(output_filename)
