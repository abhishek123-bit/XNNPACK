// Copyright (c) Facebook, Inc. and its affiliates.
// All rights reserved.
//
// Copyright 2019 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#pragma once

#include <stddef.h>
#include <stdint.h>

#include <xnnpack/params.h>
#include <xnnpack/common.h>

#ifdef __cplusplus
extern "C" {
#endif


#define DECLARE_F32_AVGPOOL_MULTIPASS_UKERNEL_FUNCTION(fn_name) \
  XNN_INTERNAL void fn_name(                                    \
      size_t n,                                                 \
      size_t ks,                                                \
      size_t kc,                                                \
      const float** x,                                          \
      const float* zero,                                        \
      float* buffer,                                            \
      float* y,                                                 \
      size_t x_increment,                                       \
      size_t y_increment,                                       \
      const union xnn_f32_avgpool_params* params);

DECLARE_F32_AVGPOOL_MULTIPASS_UKERNEL_FUNCTION(xnn_f32_avgpool_ukernel_mp9p8q__neon)
DECLARE_F32_AVGPOOL_MULTIPASS_UKERNEL_FUNCTION(xnn_f32_avgpool_ukernel_mp9p8q__psimd)
DECLARE_F32_AVGPOOL_MULTIPASS_UKERNEL_FUNCTION(xnn_f32_avgpool_ukernel_mp9p8q__scalar)
DECLARE_F32_AVGPOOL_MULTIPASS_UKERNEL_FUNCTION(xnn_f32_avgpool_ukernel_mp9p8q__sse)


#define DECLARE_F32_AVGPOOL_UNIPASS_UKERNEL_FUNCTION(fn_name) \
  XNN_INTERNAL void fn_name(                                  \
      size_t n,                                               \
      size_t ks,                                              \
      size_t kc,                                              \
      const float** x,                                        \
      const float* zero,                                      \
      float* y,                                               \
      size_t x_increment,                                     \
      size_t y_increment,                                     \
      const union xnn_f32_avgpool_params* params);

DECLARE_F32_AVGPOOL_UNIPASS_UKERNEL_FUNCTION(xnn_f32_avgpool_ukernel_up9__neon)
DECLARE_F32_AVGPOOL_UNIPASS_UKERNEL_FUNCTION(xnn_f32_avgpool_ukernel_up9__psimd)
DECLARE_F32_AVGPOOL_UNIPASS_UKERNEL_FUNCTION(xnn_f32_avgpool_ukernel_up9__scalar)
DECLARE_F32_AVGPOOL_UNIPASS_UKERNEL_FUNCTION(xnn_f32_avgpool_ukernel_up9__sse)


#define DECLARE_Q8_AVGPOOL_MULTIPASS_UKERNEL_FUNCTION(fn_name)           \
  XNN_INTERNAL void fn_name(                                             \
      size_t n,                                                          \
      size_t ks,                                                         \
      size_t kc,                                                         \
      const uint8_t** x,                                                 \
      const uint8_t* zero,                                               \
      int32_t* buffer,                                                   \
      uint8_t* y,                                                        \
      size_t x_increment,                                                \
      size_t y_increment,                                                \
      const union xnn_q8_avgpool_params* params);

DECLARE_Q8_AVGPOOL_MULTIPASS_UKERNEL_FUNCTION(xnn_q8_avgpool_ukernel_mp9p8q__neon)
DECLARE_Q8_AVGPOOL_MULTIPASS_UKERNEL_FUNCTION(xnn_q8_avgpool_ukernel_mp9p8q__scalar)
DECLARE_Q8_AVGPOOL_MULTIPASS_UKERNEL_FUNCTION(xnn_q8_avgpool_ukernel_mp9p8q__sse2)


#define DECLARE_Q8_AVGPOOL_UNIPASS_UKERNEL_FUNCTION(fn_name)             \
  XNN_INTERNAL void fn_name(                                             \
      size_t n,                                                          \
      size_t ks,                                                         \
      size_t kc,                                                         \
      const uint8_t** x,                                                 \
      const uint8_t* zero,                                               \
      uint8_t* y,                                                        \
      size_t x_increment,                                                \
      size_t y_increment,                                                \
      const union xnn_q8_avgpool_params* params);

DECLARE_Q8_AVGPOOL_UNIPASS_UKERNEL_FUNCTION(xnn_q8_avgpool_ukernel_up9__neon)
DECLARE_Q8_AVGPOOL_UNIPASS_UKERNEL_FUNCTION(xnn_q8_avgpool_ukernel_up9__scalar)
DECLARE_Q8_AVGPOOL_UNIPASS_UKERNEL_FUNCTION(xnn_q8_avgpool_ukernel_up9__sse2)


#ifdef __cplusplus
}  // extern "C"
#endif
