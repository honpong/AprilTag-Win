#!/usr/bin/python
from __future__ import print_function
from numpy import *
from numpy.linalg import norm, inv, lstsq
import sys

if len(sys.argv) != 2:
    print("Usage:", sys.argv[0], "/path/to/accel.txt")
    sys.exit(1)

g = 9.80665 # matches what we use in filter.h and state_motion.h
buckets = [[g, 0, 0], [-g, 0, 0],
           [0, g, 0], [0, -g, 0],
           [0, 0, g], [0, 0, -g]]

max_norm = norm(array([0.5, 0.5, 0.5]))

measurements = [[], [], [], [], [], []];
import csv
with open(sys.argv[1], 'rb') as csvfile:
    reader = csv.reader(csvfile)
    rnum = 0
    for row in reader:
        M = array([float(row[1]), float(row[2]), float(row[3])])
        for i in range(0, len(buckets)):
            if norm(M - buckets[i]) < max_norm:
                measurements[i].append(M)
        
        rnum += 1

mlen =  array([len(meas) for meas in measurements])
print(mlen)
print(rnum, "rows and", mlen.sum(), "measurements to use")

nrows = mlen.sum()
w = zeros([nrows, 4])
Y = zeros([nrows, 3])
row = 0
for i in range(0, len(buckets)):
    for m in measurements[i]:
        w[row, 0] = m[0]
        w[row, 1] = m[1]
        w[row, 2] = m[2]
        w[row, 3] = -1
        Y[row, 0] = buckets[i][0]
        Y[row, 1] = buckets[i][1]
        Y[row, 2] = buckets[i][2]
        row += 1

X, residuals, rank, singular = lstsq(w, Y)
print(X)
print("residuals:", residuals)
print("rank:", rank)
print("singular:", singular)
"""
wtw = dot(transpose(w),w)
wtwi = inv(wtw)
print wtwi
X = dot(wtwi, Y)
print X
"""
