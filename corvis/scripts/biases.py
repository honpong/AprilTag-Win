#!/usr/bin/env python
import json
from collections import defaultdict
from numpy import *
import sys

if len(sys.argv) <= 1:
  print "Usage:", sys.argv[0], "<cal1> (<cal2> ... <caln>)"
  sys.exit(1)

filenames = sys.argv[1:]

biases = defaultdict(list)

for fn in filenames:
  cal = json.loads(open(fn).read())
  for key in cal:
      if str(key).find('bias') != -1:
          biases[key].append(cal[key])

keys = biases.keys()
keys.sort()
for key in keys:
  print key, average(biases[key]), std(biases[key])
  print "\t", ", ".join(["%.2g" % (i) for i in biases[key]])
