#!/bin/bash

python3 ./py/translatev1.py $1

rm -rf output 2>/dev/null
mkdir output

python3 ./py/csvrwv1.py $1
