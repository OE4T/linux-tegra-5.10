#!/bin/bash
#
# Copyright (c) 2019, NVIDIA CORPORATION.  All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.

#
# Execute the unit test. Args to this script are passed on to the unit test
# core. This just serves to set the LD_LIBRARY_PATH environment variable such
# that unit tests are found and nvgpu-drv-igpu is found.
#

options=$(getopt -o t: --long test-level: -- "$@")

this_script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" > /dev/null && pwd )"
pushd $this_script_dir

if [ -f nvgpu_unit ]; then
        # if the executable is in the current directory, we are running on
        # target, so use that dir structure
        LD_LIBRARY_PATH=".:units/igpu"
        cores=$(cat /proc/cpuinfo |grep processor |wc -l)

	# Ignore number of cores for now; it seems that the parallel unit
	# tests are just too buggy and that they really don't save much
	# actual computing time.
        NVGPU_UNIT="./nvgpu_unit --nvtest --unit-load-path units/igpu --no-color \
                 --num-threads 1"
else
        # running on host
        LD_LIBRARY_PATH="build:build/units"
        # On host, must run single-threaded to avoid high VAs
        NVGPU_UNIT="./build/nvgpu_unit --num-threads 1 -r required_tests.ini"
fi
export LD_LIBRARY_PATH

echo "$ $NVGPU_UNIT $*"

$NVGPU_UNIT $*
rc=$?
if [ $rc -eq "0" ]; then
        #get the test level, if passed into this script
        eval set -- $options
        while true; do
               case "$1" in
               -t|--test-level)
                      shift;
                      testlevelparam="-t $1"
                      ;;
                --)
                      shift;
                      break;
                      ;;
                esac
                shift
        done
fi
popd
exit $rc
