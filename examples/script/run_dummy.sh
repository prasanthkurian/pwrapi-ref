#!/bin/sh

#
# Copyright 2014 Sandia Corporation. Under the terms of Contract
# DE-AC04-94AL85000, there is a non-exclusive license for use of this work
# by or on behalf of the U.S. Government. Export of this program may require
# a license from the United States Government.
#
# This file is part of the Power API Prototype software package. For license
# information, see the LICENSE file in the top level directory of the
# distribution.
#


ROOT=${PWD}/..

export LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${ROOT}/pow:${ROOT}/plugins"
export DYLD_LIBRARY_PATH="${DYLD_LIBRARY_PATH}:${ROOT}/pow:${ROOT}/plugins"
export POWERAPI_CONFIG="${ROOT}/examples/dummySystem.xml"
export POWERAPI_ROOT="plat.cab0.board0.node0"

./dummyTest