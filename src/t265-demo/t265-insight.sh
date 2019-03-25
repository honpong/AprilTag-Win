#!/bin/bash

python ./py/translatev1.py $1

rm -rf output 2>/dev/null
mkdir output

python ./py/csvrwv1.py $1
