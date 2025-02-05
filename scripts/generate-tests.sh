#!/bin/sh
# Copyright 2019 Google LLC
#
# This source code is licensed under the BSD-style license found in the
# LICENSE file in the root directory of this source tree.

### Tests for Q8 micro-kernels
tools/generate-gemm-test.py --spec test/q8-gemm.yaml --output test/q8-gemm.cc
tools/generate-gemm-test.py --spec test/q8-igemm.yaml --output test/q8-igemm.cc
tools/generate-dwconv-test.py --spec test/q8-dwconv.yaml --output test/q8-dwconv.cc

### Tests for packing micro-kernels
tools/generate-pack-test.py --spec test/x32-packx.yaml --output test/x32-packx.cc

