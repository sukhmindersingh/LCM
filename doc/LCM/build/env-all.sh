#!/bin/bash

NUM_PROCS=`nproc`
export LCM_DIR=`pwd`
export MODULEPATH=$LCM_DIR/LCM/doc/LCM/modulefiles:$MODULEPATH
unset https_proxy
unset http_proxy

# trilinos required before lcm
PACKAGES="trilinos lcm"
ARCHES="serial"
TOOL_CHAINS="gcc"
BUILD_TYPES="release"
