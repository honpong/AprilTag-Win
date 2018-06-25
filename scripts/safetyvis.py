#!/usr/bin/env python

import sys
import matplotlib
# Force matplotlib to not use any Xwindows backend.
matplotlib.use('Agg')
import matplotlib.pyplot as plt
from tumview import read_tum

if len(sys.argv) != 6:
    print "Usage:", sys.argv[0], "<centered_gt.tum> <centered_tm2.tum> <radius_m> <margin_m> <output.pdf>"
    sys.exit(1)

[gt_name, gt] = read_tum(sys.argv[1])
[tm2_name, tm2] = read_tum(sys.argv[2])
radius = float(sys.argv[3])
margin = float(sys.argv[4])
output_filename = sys.argv[5]

if 0 not in gt or 0 not in tm2:
    print "Missing device id 0 in GT or TM2"
    sys.exit(1)

fig, ax = plt.subplots()
X = 1
Y = 2
ax.plot(tm2[0][X], tm2[0][Y], label = 'tm2', linewidth=1)
ax.plot(gt [0][X], gt [0][Y], label = 'gt', linewidth=1)
ax.set_aspect('equal', 'datalim')
ax.legend()
circle_radius = plt.Circle((0, 0), radius, color='r', fill=False)
circle_margin = plt.Circle((0, 0), radius - margin, color='r', fill=False)
circle_margin2 = plt.Circle((0, 0), radius - margin*2, color='r', fill=False)

ax.add_artist(circle_radius)
ax.add_artist(circle_margin)
ax.add_artist(circle_margin2)

fig.savefig(output_filename)
