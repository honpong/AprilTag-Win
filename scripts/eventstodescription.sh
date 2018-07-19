#!/bin/bash
#
# Usage: ./eventstodescription.sh [prefix]
# 
# prefix: (default SF): prefix of the events to output.
#
# This script outputs the list of trace macros we use in tracker with
# their actual values. This is useful to generate the description.txt
# file required by traceview.
#
# Example:
# ./eventstodescription.sh SF > description.txt
# 

CC=gcc
PREFIX=SF
if [ $# -gt 0 ]; then
    PREFIX="$1"
fi

CWD=$(dirname $0)
TRACKER_ROOT=$CWD/../
TRACE_H=${TRACKER_ROOT}/src/tracker/Trace.h
MVTRACE_H=${TRACKER_ROOT}/src/movidius/device/common/mv_trace.h

TMP=$(mktemp -d 2>/dev/null || mktemp -d -t 'tmp')
SRC=$TMP/aux.cpp
EXE=$TMP/show

egrep "^#define" $MVTRACE_H > $SRC
egrep "^#define" $TRACE_H >> $SRC

tokens=$(egrep "^#define" $MVTRACE_H $TRACE_H | awk -F'[\ \(,]' '{ print $2 }' | egrep "^$PREFIX")
echo "#include <stdio.h>" >> $SRC
echo "int main() {" >> $SRC
for token in $tokens; do
    echo "    printf(\"%#04x    $token\n\", $token);" >> $SRC
done
echo "}" >> $SRC

$CC $SRC -o $EXE 2> $TMP/log
if [ $? -ne 0 ]; then
    echo "Command $CC failed, I can't continue. The log is in $TMP/log"
    exit 1
fi
$EXE
