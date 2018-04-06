#!/bin/bash

DATADIR=output

# requires you to be in a TM2 repo and have compiled tracestat
#../../../../../../tools/trace_tools/tracestat/tracestat $DATADIR/tracebuff_los.bin $DATADIR/tracebuff_lrt.bin \
../../../../../../tools/trace_tools/tracestat/tracestat $DATADIR/tracebuff_los.bin \
-p image-meas:11000 \
-p fast-path-catchup:11028 \
-p detect:11017 \
-p track:11018 \
-p stereo-detect1:11021 \
-p stereo-detect2:11026 \
-p stereo-meas:11022 \
-p stereo-match:11023 \
-p stereo-receive:11025 \
-p accel-meas:11003 \
-p gyro-meas:11004 \
-p mini-accel:11001 \
-p mini-gyro:11002 \
-p gemm:11005 \
-p gemm-shave:11006 \
-p chlesky:11007 \
-p cholesky-shave:11008 \
-p project-cov:11027

