// Copyright (c) Facebook, Inc. and its affiliates.
// All rights reserved.
//
// Copyright 2019 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <xnnpack.h>
#include <xnnpack/allocator.h>
#include <xnnpack/operator.h>
#include <xnnpack/log.h>
#include <xnnpack/common.h>
#include <xnnpack/math.h>
#include <xnnpack/params.h>
#include <xnnpack/indirection.h>


static inline size_t compute_output_dimension(
    size_t padded_input_dimension,
    size_t kernel_dimension,
    size_t dilation_dimension,
    size_t stride_dimension)
{
  const size_t effective_kernel_dimension = (kernel_dimension - 1) * dilation_dimension + 1;
  return (padded_input_dimension - effective_kernel_dimension) / stride_dimension + 1;
}

enum xnn_status xnn_create_max_pooling2d_nhwc_u8(
    uint32_t input_padding_top,
    uint32_t input_padding_right,
    uint32_t input_padding_bottom,
    uint32_t input_padding_left,
    uint32_t pooling_height,
    uint32_t pooling_width,
    uint32_t stride_height,
    uint32_t stride_width,
    uint32_t dilation_height,
    uint32_t dilation_width,
    size_t channels,
    size_t input_pixel_stride,
    size_t output_pixel_stride,
    uint8_t output_min,
    uint8_t output_max,
    uint32_t flags,
    xnn_operator_t* max_pooling_op_out)
{
  xnn_operator_t max_pooling_op = NULL;
  enum xnn_status status = xnn_status_uninitialized;

  if (!xnn_params.initialized) {
    xnn_log_error("failed to create Max Pooling operator: XNNPACK is not initialized");
    goto error;
  }

  status = xnn_status_invalid_parameter;

  const uint32_t pooling_size = pooling_height * pooling_width;
  if (pooling_size == 0) {
    xnn_log_error(
      "failed to create Max Pooling operator with %" PRIu32 "x%" PRIu32 " pooling size: "
      "pooling size dimensions must be non-zero",
      pooling_width, pooling_height);
    goto error;
  }

  if (pooling_size == 1) {
    xnn_log_error(
      "failed to create Max Pooling operator with 1 pooling element: 1x1 pooling is meaningless");
    goto error;
  }

  if (stride_height == 0 || stride_width == 0) {
    xnn_log_error(
      "failed to create Max Pooling operator with %" PRIu32 "x%" PRIu32 " stride: "
      "stride dimensions must be non-zero",
      stride_width, stride_height);
    goto error;
  }

  if (dilation_height == 0 || dilation_width == 0) {
    xnn_log_error(
      "failed to create Max Pooling operator with %" PRIu32 "x%" PRIu32 " dilation: "
      "dilation dimensions must be non-zero",
      dilation_width, dilation_height);
    goto error;
  }

  if (channels == 0) {
    xnn_log_error(
      "failed to create Max Pooling operator with %zu channels: number of channels must be non-zero",
      channels);
    goto error;
  }

  if (input_pixel_stride < channels) {
    xnn_log_error(
      "failed to create Max Pooling operator with input pixel stride of %zu: "
      "stride must be at least as large as the number of channels (%zu)",
      input_pixel_stride, channels);
    goto error;
  }

  if (output_pixel_stride < channels) {
    xnn_log_error(
      "failed to create Max Pooling operator with output pixel stride of %zu: "
      "stride must be at least as large as the number of channels (%zu)",
      output_pixel_stride, channels);
    goto error;
  }

  if (output_min >= output_max) {
    xnn_log_error(
      "failed to create Max Pooling operator with [%" PRIu8 ", %" PRIu8 "] output range: "
      "range min must be below range max",
      output_min, output_max);
    goto error;
  }

  status = xnn_status_out_of_memory;

  max_pooling_op = xnn_allocate_zero_memory(sizeof(struct xnn_operator));
  if (max_pooling_op == NULL) {
    xnn_log_error("failed to allocate %zu bytes for Max Pooling operator descriptor", sizeof(struct xnn_operator));
    goto error;
  }

  max_pooling_op->padding_top = input_padding_top;
  max_pooling_op->padding_right = input_padding_right;
  max_pooling_op->padding_bottom = input_padding_bottom;
  max_pooling_op->padding_left = input_padding_left;

  max_pooling_op->kernel_height = pooling_height;
  max_pooling_op->kernel_width = pooling_width;
  max_pooling_op->stride_height = stride_height;
  max_pooling_op->stride_width = stride_width;
  max_pooling_op->dilation_height = dilation_height;
  max_pooling_op->dilation_width = dilation_width;
  max_pooling_op->channels = channels;
  max_pooling_op->input_pixel_stride = input_pixel_stride;
  max_pooling_op->output_pixel_stride = output_pixel_stride;

  max_pooling_op->u8_output_params = xnn_compute_u8_output_params(output_min, output_max);

  max_pooling_op->type = xnn_operator_type_max_pooling_u8;
  max_pooling_op->ukernel.type = xnn_ukernel_type_max_pooling;

  max_pooling_op->state = xnn_run_state_invalid;

  *max_pooling_op_out = max_pooling_op;
  return xnn_status_success;

error:
  xnn_delete_operator(max_pooling_op);
  return status;
}

enum xnn_status xnn_create_max_pooling2d_nhwc_f32(
    uint32_t input_padding_top,
    uint32_t input_padding_right,
    uint32_t input_padding_bottom,
    uint32_t input_padding_left,
    uint32_t pooling_height,
    uint32_t pooling_width,
    uint32_t stride_height,
    uint32_t stride_width,
    uint32_t dilation_height,
    uint32_t dilation_width,
    size_t channels,
    size_t input_pixel_stride,
    size_t output_pixel_stride,
    float output_min,
    float output_max,
    uint32_t flags,
    xnn_operator_t* max_pooling_op_out)
{
  xnn_operator_t max_pooling_op = NULL;
  enum xnn_status status = xnn_status_uninitialized;

  if (!xnn_params.initialized) {
    xnn_log_error("failed to setup Max Pooling operator: XNNPACK is not initialized");
    return xnn_status_uninitialized;
  }

  status = xnn_status_invalid_parameter;

  const uint32_t pooling_size = pooling_height * pooling_width;
  if (pooling_size == 0) {
    xnn_log_error(
      "failed to create Max Pooling operator with %" PRIu32 "x%" PRIu32 " pooling size: "
      "pooling size dimensions must be non-zero",
      pooling_width, pooling_height);
    goto error;
  }

  if (pooling_size == 1) {
    xnn_log_error(
      "failed to create Max Pooling operator with 1 pooling element: "
      "1x1 pooling is meaningless");
    goto error;
  }

  if (stride_height == 0 || stride_width == 0) {
    xnn_log_error(
      "failed to create Max Pooling operator with %" PRIu32 "x%" PRIu32 " stride: "
      "stride dimensions must be non-zero",
      stride_width, stride_height);
    goto error;
  }

  if (dilation_height == 0 || dilation_width == 0) {
    xnn_log_error(
      "failed to create Max Pooling operator with %" PRIu32 "x%" PRIu32 " dilation: "
      "dilation dimensions must be non-zero",
      dilation_width, dilation_height);
    goto error;
  }

  if (channels == 0) {
    xnn_log_error(
      "failed to create Max Pooling operator with %zu channels: number of channels must be non-zero",
      channels);
    goto error;
  }

  if (input_pixel_stride < channels) {
    xnn_log_error(
      "failed to create Max Pooling operator with input pixel stride of %zu: "
      "stride must be at least as large as the number of channels (%zu)",
      input_pixel_stride, channels);
    goto error;
  }

  if (output_pixel_stride < channels) {
    xnn_log_error(
      "failed to create Max Pooling operator with output pixel stride of %zu: "
      "stride must be at least as large as the number of channels (%zu)",
      output_pixel_stride, channels);
    goto error;
  }

  if (isnan(output_min)) {
    xnn_log_error(
      "failed to create Max Pooling with NaN output lower bound: lower bound must be non-NaN");
    goto error;
  }

  if (isnan(output_max)) {
    xnn_log_error(
      "failed to create Max Pooling with NaN output upper bound: upper bound must be non-NaN");
    goto error;
  }

  if (output_min >= output_max) {
    xnn_log_error(
      "failed to create Max Pooling with [%.7g, %.7g] output range: lower bound must be below upper bound",
      output_min, output_max);
    goto error;
  }

  status = xnn_status_out_of_memory;

  max_pooling_op = xnn_allocate_zero_memory(sizeof(struct xnn_operator));
  if (max_pooling_op == NULL) {
    xnn_log_error("failed to allocate %zu bytes for Max Pooling operator descriptor", sizeof(struct xnn_operator));
    goto error;
  }

  max_pooling_op->padding_top = input_padding_top;
  max_pooling_op->padding_right = input_padding_right;
  max_pooling_op->padding_bottom = input_padding_bottom;
  max_pooling_op->padding_left = input_padding_left;

  max_pooling_op->kernel_height = pooling_height;
  max_pooling_op->kernel_width = pooling_width;
  max_pooling_op->stride_height = stride_height;
  max_pooling_op->stride_width = stride_width;
  max_pooling_op->dilation_height = dilation_height;
  max_pooling_op->dilation_width = dilation_width;
  max_pooling_op->channels = channels;
  max_pooling_op->input_pixel_stride = input_pixel_stride;
  max_pooling_op->output_pixel_stride = output_pixel_stride;

  max_pooling_op->f32_output_params = xnn_compute_f32_output_params(output_min, output_max);

  max_pooling_op->type = xnn_operator_type_max_pooling_f32;
  max_pooling_op->ukernel.type = xnn_ukernel_type_max_pooling;

  max_pooling_op->state = xnn_run_state_invalid;

  *max_pooling_op_out = max_pooling_op;
  return xnn_status_success;

error:
  xnn_delete_operator(max_pooling_op);
  return status;
}

enum xnn_status xnn_setup_max_pooling2d_nhwc_u8(
    xnn_operator_t max_pooling_op,
    size_t batch_size,
    size_t input_height,
    size_t input_width,
    const uint8_t* input,
    uint8_t* output,
    pthreadpool_t threadpool)
{
  if (max_pooling_op->type != xnn_operator_type_max_pooling_u8) {
    xnn_log_error("failed to setup Max Pooling (U8) operator: operator type mismatch");
    return xnn_status_invalid_parameter;
  }
  max_pooling_op->state = xnn_run_state_invalid;

  if (!xnn_params.initialized) {
    xnn_log_error("failed to setup Max Pooling operator: XNNPACK is not initialized");
    return xnn_status_uninitialized;
  }

  if (input_width == 0 || input_height == 0) {
    xnn_log_error(
      "failed to setup Max Pooling operator with %zux%zu input: input dimensions must be non-zero",
      input_width, input_height);
    return xnn_status_invalid_parameter;
  }

  if (batch_size == 0) {
    max_pooling_op->state = xnn_run_state_skip;
    return xnn_status_success;
  }

  max_pooling_op->batch_size = batch_size;
  max_pooling_op->input_height = input_height;
  max_pooling_op->input_width = input_width;
  max_pooling_op->input = input;

  max_pooling_op->output_height = compute_output_dimension(
      max_pooling_op->padding_top + input_height + max_pooling_op->padding_bottom,
      max_pooling_op->kernel_height,
      max_pooling_op->dilation_height,
      max_pooling_op->stride_height);
  max_pooling_op->output_width = compute_output_dimension(
      max_pooling_op->padding_left + input_width + max_pooling_op->padding_right,
      max_pooling_op->kernel_width,
      max_pooling_op->dilation_width,
      max_pooling_op->stride_width);
  max_pooling_op->output = output;

  size_t valid_batch_size = 0;
  if (input == max_pooling_op->last_input &&
      input_height == max_pooling_op->last_input_height &&
      input_width == max_pooling_op->last_input_width)
  {
    valid_batch_size = max_pooling_op->valid_batch_size;
    if (batch_size <= valid_batch_size) {
      max_pooling_op->compute.range[0] = batch_size;
      max_pooling_op->state = xnn_run_state_ready;
      return xnn_status_success;
    }
  }

  const size_t pooling_height = max_pooling_op->kernel_height;
  const size_t pooling_width = max_pooling_op->kernel_width;
  const size_t pooling_size = pooling_height * pooling_width;
  const size_t output_height = max_pooling_op->output_height;
  const size_t output_width = max_pooling_op->output_width;
  // Micro-kernel may read up to (mr - 1) elements after the end of indirection buffer.
  const uint32_t mr = xnn_params.u8.maxpool.mr;

  const size_t step_width =
    max_pooling_op->dilation_width > 1 ? pooling_width : min(max_pooling_op->stride_width, pooling_width);
  const size_t step_height = pooling_size + (output_width * step_width - 1) * pooling_height;
  const size_t indirection_buffer_size = sizeof(void*) * ((mr - 1) + batch_size * output_height * step_height);

  const void** indirection_buffer = (const void**) realloc(max_pooling_op->indirection_buffer, indirection_buffer_size);
  if (indirection_buffer == NULL) {
    xnn_log_error("failed to allocate %zu bytes for indirection buffer", indirection_buffer_size);
    return xnn_status_out_of_memory;
  }
  max_pooling_op->indirection_buffer = indirection_buffer;

  xnn_indirection_init_maxpool2d(max_pooling_op, valid_batch_size, step_height, step_width, 0 /* log2(sizeof(uint8_t)) */);

  const uint32_t qr = xnn_params.u8.maxpool.qr;
  const size_t channels = max_pooling_op->channels;

  const size_t indirect_input_height_stride = step_height * sizeof(void*);
  const size_t output_width_stride = max_pooling_op->output_pixel_stride * sizeof(uint8_t);
  const size_t output_height_stride = output_width * output_width_stride;
  const size_t multipass_adjustment = round_up(doz(pooling_size, mr), qr) + mr;

  max_pooling_op->context.max_pooling = (struct max_pooling_context) {
      .indirect_input = indirection_buffer,
      .indirect_input_batch_stride = output_height * indirect_input_height_stride,
      .indirect_input_height_stride = indirect_input_height_stride,
      .output = output,
      .output_batch_stride = output_height * output_height_stride,
      .output_height_stride = output_height_stride,
      .output_width = output_width,
      .pooling_size = pooling_size,
      .channels = channels,
      .input_increment = (pooling_height * step_width - multipass_adjustment) * sizeof(void*),
      .output_increment = output_width_stride - channels * sizeof(uint8_t),
      .params.u8 = max_pooling_op->u8_output_params,
      .ukernel = xnn_params.u8.maxpool.ukernel,
  };
  max_pooling_op->compute.type = xnn_parallelization_type_2d;
  max_pooling_op->compute.task_2d = (pthreadpool_task_2d_t) xnn_compute_max_pooling;
  max_pooling_op->compute.range[0] = batch_size;
  max_pooling_op->compute.range[1] = output_height;
  max_pooling_op->state = xnn_run_state_ready;

  max_pooling_op->last_input = input;
  max_pooling_op->last_input_height = input_height;
  max_pooling_op->last_input_width = input_width;
  max_pooling_op->valid_batch_size = max(valid_batch_size, batch_size);

  return xnn_status_success;
}

enum xnn_status xnn_setup_max_pooling2d_nhwc_f32(
    xnn_operator_t max_pooling_op,
    size_t batch_size,
    size_t input_height,
    size_t input_width,
    const float* input,
    float* output,
    pthreadpool_t threadpool)
{
  if (max_pooling_op->type != xnn_operator_type_max_pooling_f32) {
    xnn_log_error("failed to setup Max Pooling (F32) operator: operator type mismatch");
    return xnn_status_invalid_parameter;
  }
  max_pooling_op->state = xnn_run_state_invalid;

  if (!xnn_params.initialized) {
    xnn_log_error(
      "failed to setup Max Pooling operator: XNNPACK is not initialized");
    return xnn_status_uninitialized;
  }

  if (input_width == 0 || input_height == 0) {
    xnn_log_error(
      "failed to setup Max Pooling operator with %zux%zu input: input dimensions must be non-zero",
      input_width, input_height);
    return xnn_status_invalid_parameter;
  }

  if (batch_size == 0) {
    max_pooling_op->state = xnn_run_state_skip;
    return xnn_status_success;
  }

  max_pooling_op->batch_size = batch_size;
  max_pooling_op->input_height = input_height;
  max_pooling_op->input_width = input_width;
  max_pooling_op->input = input;

  max_pooling_op->output_height = compute_output_dimension(
      max_pooling_op->padding_top + input_height + max_pooling_op->padding_bottom,
      max_pooling_op->kernel_height,
      max_pooling_op->dilation_height,
      max_pooling_op->stride_height);
  max_pooling_op->output_width = compute_output_dimension(
      max_pooling_op->padding_left + input_width + max_pooling_op->padding_right,
      max_pooling_op->kernel_width,
      max_pooling_op->dilation_width,
      max_pooling_op->stride_width);
  max_pooling_op->output = output;

  size_t valid_batch_size = 0;
  if (input == max_pooling_op->last_input &&
      input_height == max_pooling_op->last_input_height &&
      input_width == max_pooling_op->last_input_width)
  {
    valid_batch_size = max_pooling_op->valid_batch_size;
    if (batch_size <= valid_batch_size) {
      max_pooling_op->compute.range[0] = batch_size;
      max_pooling_op->state = xnn_run_state_ready;
      return xnn_status_success;
    }
  }

  const size_t pooling_height = max_pooling_op->kernel_height;
  const size_t pooling_width = max_pooling_op->kernel_width;
  const size_t pooling_size = pooling_height * pooling_width;
  const size_t output_height = max_pooling_op->output_height;
  const size_t output_width = max_pooling_op->output_width;
  // Micro-kernel may read up to (mr - 1) elements after the end of indirection buffer.
  const uint32_t mr = xnn_params.f32.maxpool.mr;

  const size_t step_width =
    max_pooling_op->dilation_width > 1 ? pooling_width : min(max_pooling_op->stride_width, pooling_width);
  const size_t step_height = pooling_size + (output_width * step_width - 1) * pooling_height;
  const size_t indirection_buffer_size = sizeof(void*) * ((mr - 1) + batch_size * output_height * step_height);

  const void** indirection_buffer = (const void**) realloc(max_pooling_op->indirection_buffer, indirection_buffer_size);
  if (indirection_buffer == NULL) {
    xnn_log_error("failed to allocate %zu bytes for indirection buffer", indirection_buffer_size);
    return xnn_status_out_of_memory;
  }
  max_pooling_op->indirection_buffer = indirection_buffer;

  xnn_indirection_init_maxpool2d(max_pooling_op, valid_batch_size, step_height, step_width, 2 /* log2(sizeof(float)) */);

  const uint32_t qr = xnn_params.f32.maxpool.qr;
  const size_t channels = max_pooling_op->channels;

  const size_t indirect_input_height_stride = step_height * sizeof(void*);
  const size_t output_width_stride = max_pooling_op->output_pixel_stride * sizeof(float);
  const size_t output_height_stride = output_width * output_width_stride;
  const size_t multipass_adjustment = round_up(doz(pooling_size, mr), qr) + mr;

  max_pooling_op->context.max_pooling = (struct max_pooling_context) {
      .indirect_input = indirection_buffer,
      .indirect_input_batch_stride = output_height * indirect_input_height_stride,
      .indirect_input_height_stride = indirect_input_height_stride,
      .output = output,
      .output_batch_stride = output_height * output_height_stride,
      .output_height_stride = output_height_stride,
      .output_width = output_width,
      .pooling_size = pooling_size,
      .channels = channels,
      .input_increment = (pooling_height * step_width - multipass_adjustment) * sizeof(void*),
      .output_increment = output_width_stride - channels * sizeof(float),
      .params.f32 = max_pooling_op->f32_output_params,
      .ukernel = xnn_params.f32.maxpool.ukernel,
  };
  max_pooling_op->compute.type = xnn_parallelization_type_2d;
  max_pooling_op->compute.task_2d = (pthreadpool_task_2d_t) xnn_compute_max_pooling;
  max_pooling_op->compute.range[0] = batch_size;
  max_pooling_op->compute.range[1] = output_height;
  max_pooling_op->state = xnn_run_state_ready;

  max_pooling_op->last_input = input;
  max_pooling_op->last_input_height = input_height;
  max_pooling_op->last_input_width = input_width;
  max_pooling_op->valid_batch_size = max(valid_batch_size, batch_size);

  return xnn_status_success;
}
