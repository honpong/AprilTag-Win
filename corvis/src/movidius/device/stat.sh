#!/bin/bash

DATADIR=output

# requires you to be in a TM2 repo and have compiled tracestat
#../../../../../../tools/trace_tools/tracestat/tracestat $DATADIR/tracebuff_los.bin $DATADIR/tracebuff_lrt.bin \
../../../../../../tools/trace_tools/tracestat/tracestat $DATADIR/tracebuff_los.bin \
-p image-meas:11000 \
-p accel-meas:11003 \
-p gyro-meas:11004 \
-p mini-accel:11001 \
-p mini-gyro:11002 \
-p gyro-rec:10027 \
-p accel-rec:10026 \
-p image-rec:10025 \
-p stereo-rec:10043 \
-p gemm:11005 \
-p gemm-shave:11006 \
-p chlesky:11007 \
-p cholesky-shave:11008 \
-p shave-track:10032 \
-p shave-detect:10033 \
-p project-cov:10027

