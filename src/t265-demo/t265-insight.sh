#!/bin/bash

#./translatev1 $1
python3 ./py/translatev1.py $1

rm -rf output 2>/dev/null
mkdir output

#./csvrwv1 $1
python3 ./py/csvrwv1.py $1
