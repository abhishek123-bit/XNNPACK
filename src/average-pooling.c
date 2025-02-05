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
    size_t pooling_dimension,
    size_t stride_dimension)
{
  return (padded_input_dimension - pooling_dimension) / stride_dimension + 1;
}

enum xnn_status xnn_create_average_pooling2d_nhwc_q8(
    uint32_t input_padding_top,
    uint32_t input_padding_right,
    uint32_t input_padding_bottom,
    uint32_t input_padding_left,
    uint32_t pooling_height,
    uint32_t pooling_width,
    uint32_t stride_height,
    uint32_t stride_width,
    size_t channels,
    size_t input_pixel_stride,
    size_t output_pixel_stride,
    uint8_t input_zero_point,
    float input_scale,
    uint8_t output_zero_point,
    float output_scale,
    uint8_t output_min,
    uint8_t output_max,
    uint32_t flags,
    xnn_operator_t* average_pooling_op_out)
{
  xnn_operator_t average_pooling_op = NULL;
  enum xnn_status status = xnn_status_uninitialized;

  if (!xnn_params.initialized) {
    xnn_log_error("failed to create Average Pooling operator: XNNPACK is not initialized");
    goto error;
  }

  status = xnn_status_invalid_parameter;

  const uint32_t pooling_size = pooling_height * pooling_width;
  if (pooling_size == 0) {
    xnn_log_error(
      "failed to create Average Pooling operator with %" PRIu32 "x%" PRIu32 " pooling size: "
      "pooling size dimensions must be non-zero",
      pooling_width, pooling_height);
    goto error;
  }

  if (pooling_size == 1) {
    xnn_log_error(
      "failed to create Average Pooling operator with 1 pooling element: 1x1 pooling is meaningless");
    goto error;
  }

  if (stride_height == 0 || stride_width == 0) {
    xnn_log_error(
      "failed to create Average Pooling operator with %" PRIu32 "x%" PRIu32 " stride: "
      "stride dimensions must be non-zero",
      stride_width, stride_height);
    goto error;
  }

  if (channels == 0) {
    xnn_log_error(
      "failed to create Average Pooling operator with %zu channels: number of channels must be non-zero",
      channels);
    goto error;
  }

  if (input_pixel_stride < channels) {
    xnn_log_error(
      "failed to create Average Pooling operator with input pixel stride of %zu: "
      "stride must be at least as large as the number of channels (%zu)",
      input_pixel_stride, channels);
    goto error;
  }

  if (output_pixel_stride < channels) {
    xnn_log_error(
      "failed to create Average Pooling operator with output pixel stride of %zu: "
      "stride must be at least as large as the number of channels (%zu)",
      output_pixel_stride, channels);
    goto error;
  }

  if (input_scale <= 0.0f || !isnormal(input_scale)) {
    xnn_log_error(
      "failed to create Average Pooling operator with %.7g input scale: "
      "scale must be finite, normalized, and positive",
      input_scale);
    goto error;
  }

  if (output_scale <= 0.0f || !isnormal(output_scale)) {
    xnn_log_error(
      "failed to create Average Pooling operator with %.7g output scale: "
      "scale must be finite, normalized, and positive",
      output_scale);
    goto error;
  }

  if (output_min >= output_max) {
    xnn_log_error(
      "failed to create Average Pooling operator with [%" PRIu8 ", %" PRIu8 "] output range: "
      "range min must be below range max",
      output_min, output_max);
    goto error;
  }

  status = xnn_status_unsupported_parameter;

  const float input_output_scale = input_scale / output_scale;
  if (input_output_scale < 0x1.0p-8f || input_output_scale >= 0x1.0p+8f) {
    xnn_log_error(
      "failed to create Average Pooling operator with %.7g input scale and %.7g output scale: "
      "input-to-output scale ratio (%.7f) must be in [2**-8, 2**8) range",
      input_scale, output_scale, input_output_scale);
    goto error;
  }

  if (pooling_size >= 16777216) {
    xnn_log_error(
      "failed to create Average Pooling operator with %"PRIu32" (%" PRIu32 "x%" PRIu32 ") pooling elements: "
      "the number of elements in the pooling area must be below 2**24",
      pooling_size, pooling_width, pooling_height);
    goto error;
  }

  status = xnn_status_out_of_memory;

  average_pooling_op = xnn_allocate_zero_memory(sizeof(struct xnn_operator));
  if (average_pooling_op == NULL) {
    xnn_log_error("failed to allocate %zu bytes for Average Pooling operator descriptor", sizeof(struct xnn_operator));
    goto error;
  }

  const bool any_padding = (input_padding_left | input_padding_top | input_padding_right | input_padding_bottom) != 0;
  const uint32_t mr = xnn_params.q8.avgpool.mr;
  const uint32_t qr = xnn_params.q8.avgpool.qr;
  if (any_padding || pooling_size < mr || (pooling_size - mr) % qr != 0) {
    void* zero_buffer = xnn_allocate_memory(channels * sizeof(uint8_t) + XNN_EXTRA_BYTES);
    if (zero_buffer == NULL) {
      xnn_log_error("failed to allocate %zu bytes for Average Pooling zero padding",
        channels * sizeof(uint8_t) + XNN_EXTRA_BYTES);
      goto error;
    }
    memset(zero_buffer, input_zero_point, channels * sizeof(uint8_t));
    average_pooling_op->zero_buffer = zero_buffer;
  }

  average_pooling_op->padding_top = input_padding_top;
  average_pooling_op->padding_right = input_padding_right;
  average_pooling_op->padding_bottom = input_padding_bottom;
  average_pooling_op->padding_left = input_padding_left;

  average_pooling_op->kernel_height = pooling_height;
  average_pooling_op->kernel_width = pooling_width;
  average_pooling_op->stride_height = stride_height;
  average_pooling_op->stride_width = stride_width;
  average_pooling_op->dilation_height = 1;
  average_pooling_op->dilation_width = 1;
  average_pooling_op->channels = channels;
  average_pooling_op->input_pixel_stride = input_pixel_stride;
  average_pooling_op->output_pixel_stride = output_pixel_stride;

  // Number of rows read in the micro-kernel.
  const size_t nrows = round_up(doz(pooling_size, mr), qr) + mr;
  average_pooling_op->q8_avgpool_params =
    xnn_compute_q8_avgpool_params(
      (int32_t) -((uint32_t) input_zero_point * (uint32_t) nrows),
      input_scale / (output_scale * (float) pooling_size),
      output_zero_point, output_min, output_max);

  average_pooling_op->type = xnn_operator_type_average_pooling_q8;
  average_pooling_op->ukernel.type = xnn_ukernel_type_average_pooling;

  *average_pooling_op_out = average_pooling_op;
  return xnn_status_success;

error:
  xnn_delete_operator(average_pooling_op);
  return status;
}

enum xnn_status xnn_create_average_pooling2d_nhwc_f32(
    uint32_t input_padding_top,
    uint32_t input_padding_right,
    uint32_t input_padding_bottom,
    uint32_t input_padding_left,
    uint32_t pooling_height,
    uint32_t pooling_width,
    uint32_t stride_height,
    uint32_t stride_width,
    size_t channels,
    size_t input_pixel_stride,
    size_t output_pixel_stride,
    float output_min,
    float output_max,
    uint32_t flags,
    xnn_operator_t* average_pooling_op_out)
{
  xnn_operator_t average_pooling_op = NULL;
  enum xnn_status status = xnn_status_uninitialized;

  if (!xnn_params.initialized) {
    xnn_log_error("failed to create Average Pooling operator: XNNPACK is not initialized");
    goto error;
  }

  status = xnn_status_invalid_parameter;

  const uint32_t pooling_size = pooling_height * pooling_width;
  if (pooling_size == 0) {
    xnn_log_error(
      "failed to create Average Pooling operator with %" PRIu32 "x%" PRIu32 " pooling size: "
      "pooling size dimensions must be non-zero",
      pooling_width, pooling_height);
    goto error;
  }

  if (pooling_size == 1) {
    xnn_log_error(
      "failed to create Average Pooling operator with 1 pooling element: 1x1 pooling is meaningless");
    goto error;
  }

  if (stride_height == 0 || stride_width == 0) {
    xnn_log_error(
      "failed to create Average Pooling operator with %" PRIu32 "x%" PRIu32 " stride: "
      "stride dimensions must be non-zero",
      stride_width, stride_height);
    goto error;
  }

  if (channels == 0) {
    xnn_log_error(
      "failed to create Average Pooling operator with %zu channels: number of channels must be non-zero",
      channels);
    goto error;
  }

  if (input_pixel_stride < channels) {
    xnn_log_error(
      "failed to create Average Pooling operator with input pixel stride of %zu: "
      "stride must be at least as large as the number of channels (%zu)",
      input_pixel_stride, channels);
    goto error;
  }

  if (output_pixel_stride < channels) {
    xnn_log_error(
      "failed to create Average Pooling operator with output pixel stride of %zu: "
      "stride must be at least as large as the number of channels (%zu)",
      output_pixel_stride, channels);
    goto error;
  }

  if (isnan(output_min)) {
    xnn_log_error(
      "failed to create Average Pooling operator with NaN output lower bound: lower bound must be non-NaN");
    goto error;
  }

  if (isnan(output_max)) {
    xnn_log_error(
      "failed to create Average Pooling operator with NaN output upper bound: upper bound must be non-NaN");
    goto error;
  }

  if (output_min >= output_max) {
    xnn_log_error(
      "failed to create Average Pooling operator with [%.7g, %.7g] output range: lower bound must be below upper bound",
      output_min, output_max);
    goto error;
  }

  status = xnn_status_out_of_memory;

  average_pooling_op = xnn_allocate_zero_memory(sizeof(struct xnn_operator));
  if (average_pooling_op == NULL) {
    xnn_log_error("failed to allocate %zu bytes for Average Pooling operator descriptor", sizeof(struct xnn_operator));
    goto error;
  }

  const bool any_padding = (input_padding_left | input_padding_top | input_padding_right | input_padding_bottom) != 0;
  const uint32_t mr = xnn_params.f32.avgpool.mr;
  const uint32_t qr = xnn_params.f32.avgpool.qr;
  if (any_padding || pooling_size < mr || (pooling_size - mr) % qr != 0) {
    void* zero_buffer = xnn_allocate_memory(channels * sizeof(float) + XNN_EXTRA_BYTES);
    if (zero_buffer == NULL) {
      xnn_log_error("failed to allocate %zu bytes for Average Pooling zero padding",
        channels * sizeof(float) + XNN_EXTRA_BYTES);
      goto error;
    }
    memset(zero_buffer, 0, channels * sizeof(float));
    average_pooling_op->zero_buffer = zero_buffer;
  }

  average_pooling_op->padding_top = input_padding_top;
  average_pooling_op->padding_right = input_padding_right;
  average_pooling_op->padding_bottom = input_padding_bottom;
  average_pooling_op->padding_left = input_padding_left;

  average_pooling_op->kernel_height = pooling_height;
  average_pooling_op->kernel_width = pooling_width;
  average_pooling_op->stride_height = stride_height;
  average_pooling_op->stride_width = stride_width;
  average_pooling_op->dilation_height = 1;
  average_pooling_op->dilation_width = 1;
  average_pooling_op->channels = channels;
  average_pooling_op->input_pixel_stride = input_pixel_stride;
  average_pooling_op->output_pixel_stride = output_pixel_stride;

  average_pooling_op->type = xnn_operator_type_average_pooling_f32;
  if (any_padding) {
    average_pooling_op->f32_output_params =
      xnn_compute_f32_output_params(output_min, output_max);

    average_pooling_op->ukernel.type = xnn_ukernel_type_pixelwise_average_pooling;
  } else {
    average_pooling_op->f32_avgpool_params =
      xnn_compute_f32_avgpool_params(1.0f / (float) pooling_size, output_min, output_max);

    average_pooling_op->ukernel.type = xnn_ukernel_type_average_pooling;
  }

  *average_pooling_op_out = average_pooling_op;
  return xnn_status_success;

error:
  xnn_delete_operator(average_pooling_op);
  return status;
}

enum xnn_status xnn_setup_average_pooling2d_nhwc_q8(
    xnn_operator_t average_pooling_op,
    size_t batch_size,
    size_t input_height,
    size_t input_width,
    const uint8_t* input,
    uint8_t* output,
    pthreadpool_t threadpool)
{
  if (average_pooling_op->type != xnn_operator_type_average_pooling_q8) {
    xnn_log_error("failed to setup Average Pooling (Q8) operator: operator type mismatch");
    return xnn_status_invalid_parameter;
  }
  average_pooling_op->state = xnn_run_state_invalid;

  if (!xnn_params.initialized) {
    xnn_log_error("failed to setup Average Pooling operator: XNNPACK is not initialized");
    return xnn_status_uninitialized;
  }

  if (input_width == 0 || input_height == 0) {
    xnn_log_error(
      "failed to setup Average Pooling operator with %zux%zu input: input dimensions must be non-zero",
      input_width, input_height);
    return xnn_status_invalid_parameter;
  }

  if (batch_size == 0) {
    average_pooling_op->state = xnn_run_state_skip;
    return xnn_status_success;
  }

  average_pooling_op->batch_size = batch_size;
  average_pooling_op->input_height = input_height;
  average_pooling_op->input_width = input_width;
  average_pooling_op->input = input;

  average_pooling_op->output_height = compute_output_dimension(
      average_pooling_op->padding_top + input_height + average_pooling_op->padding_bottom,
      average_pooling_op->kernel_height,
      average_pooling_op->stride_height);
  average_pooling_op->output_width = compute_output_dimension(
      average_pooling_op->padding_left + input_width + average_pooling_op->padding_right,
      average_pooling_op->kernel_width,
      average_pooling_op->stride_width);
  average_pooling_op->output = output;

  size_t valid_batch_size = 0;
  if (input == average_pooling_op->last_input &&
      input_height == average_pooling_op->last_input_height &&
      input_width == average_pooling_op->last_input_width)
  {
    valid_batch_size = average_pooling_op->valid_batch_size;
    if (batch_size <= valid_batch_size) {
      average_pooling_op->compute.range[0] = batch_size;
      average_pooling_op->state = xnn_run_state_ready;
      return xnn_status_success;
    }
  }

  const size_t pooling_height = average_pooling_op->kernel_height;
  const size_t pooling_width = average_pooling_op->kernel_width;
  const size_t pooling_size = pooling_height * pooling_width;
  const size_t output_height = average_pooling_op->output_height;
  const size_t output_width = average_pooling_op->output_width;
  // Micro-kernel may read up to (mr - 1) elements after the end of indirection buffer.
  const uint32_t mr = xnn_params.q8.avgpool.mr;

  const size_t step_width = min(average_pooling_op->stride_width, pooling_width);
  const size_t step_height = pooling_size + (output_width * step_width - 1) * pooling_height;
  const size_t indirection_buffer_size = sizeof(void*) * ((mr - 1) + batch_size * output_height * step_height);

  const void** indirection_buffer = (const void**) realloc(average_pooling_op->indirection_buffer, indirection_buffer_size);
  if (indirection_buffer == NULL) {
    xnn_log_error("failed to allocate %zu bytes for indirection buffer", indirection_buffer_size);
    return xnn_status_out_of_memory;
  }
  average_pooling_op->indirection_buffer = indirection_buffer;

  xnn_indirection_init_dwconv2d(
    average_pooling_op, valid_batch_size, step_height, step_width, 0 /* log2(sizeof(uint8_t)) */);

  const uint32_t qr = xnn_params.q8.avgpool.qr;
  const size_t channels = average_pooling_op->channels;

  const size_t indirect_input_height_stride = step_height * sizeof(void*);
  const size_t output_width_stride = average_pooling_op->output_pixel_stride * sizeof(uint8_t);
  const size_t output_height_stride = output_width * output_width_stride;

  const size_t multipass_adjustment =
    pooling_size > mr ? round_up(pooling_size - mr, qr) + mr - qr : 0;
  average_pooling_op->context.average_pooling = (struct average_pooling_context) {
      .indirect_input = indirection_buffer,
      .indirect_input_batch_stride = output_height * indirect_input_height_stride,
      .indirect_input_height_stride = indirect_input_height_stride,
      .output = output,
      .output_batch_stride = output_height * output_height_stride,
      .output_height_stride = output_height_stride,
      .output_width = output_width,
      .pooling_size = pooling_size,
      .channels = channels,
      .zero = average_pooling_op->zero_buffer,
      .input_increment = (pooling_height * step_width - multipass_adjustment) * sizeof(void*),
      .output_increment = output_width_stride - channels * sizeof(uint8_t),
      .params.q8 = average_pooling_op->q8_avgpool_params,
  };
  average_pooling_op->compute.type = xnn_parallelization_type_2d;
  average_pooling_op->compute.range[0] = batch_size;
  average_pooling_op->compute.range[1] = output_height;

  if (pooling_size <= mr) {
    average_pooling_op->context.average_pooling.unipass_ukernel = xnn_params.q8.avgpool.up;
    average_pooling_op->compute.task_2d = (pthreadpool_task_2d_t) xnn_compute_average_pooling_unipass;
  } else {
    average_pooling_op->context.average_pooling.multipass_ukernel = xnn_params.q8.avgpool.mp;
    average_pooling_op->compute.task_2d = (pthreadpool_task_2d_t) xnn_compute_average_pooling_multipass;
  }
  average_pooling_op->state = xnn_run_state_ready;

  average_pooling_op->last_input = input;
  average_pooling_op->last_input_height = input_height;
  average_pooling_op->last_input_width = input_width;
  average_pooling_op->valid_batch_size = max(valid_batch_size, batch_size);

  return xnn_status_success;
}

enum xnn_status xnn_setup_average_pooling2d_nhwc_f32(
    xnn_operator_t average_pooling_op,
    size_t batch_size,
    size_t input_height,
    size_t input_width,
    const float* input,
    float* output,
    pthreadpool_t threadpool)
{
  if (average_pooling_op->type != xnn_operator_type_average_pooling_f32) {
    xnn_log_error("failed to setup Average Pooling (F32) operator: operator type mismatch");
    return xnn_status_invalid_parameter;
  }
  average_pooling_op->state = xnn_run_state_invalid;

  if (!xnn_params.initialized) {
    xnn_log_error("failed to setup Average Pooling operator: XNNPACK is not initialized");
    return xnn_status_uninitialized;
  }

  if (input_width == 0 || input_height == 0) {
    xnn_log_error(
      "failed to setup Average Pooling operator with %zux%zu input: input dimensions must be non-zero",
      input_width, input_height);
    return xnn_status_invalid_parameter;
  }

  if (batch_size == 0) {
    average_pooling_op->state = xnn_run_state_skip;
    return xnn_status_success;
  }

  average_pooling_op->batch_size = batch_size;
  average_pooling_op->input_height = input_height;
  average_pooling_op->input_width = input_width;
  average_pooling_op->input = input;

  average_pooling_op->output_height = compute_output_dimension(
      average_pooling_op->padding_top + input_height + average_pooling_op->padding_bottom,
      average_pooling_op->kernel_height,
      average_pooling_op->stride_height);
  average_pooling_op->output_width = compute_output_dimension(
      average_pooling_op->padding_left + input_width + average_pooling_op->padding_right,
      average_pooling_op->kernel_width,
      average_pooling_op->stride_width);
  average_pooling_op->output = output;

  size_t valid_batch_size = 0;
  if (input == average_pooling_op->last_input &&
      input_height == average_pooling_op->last_input_height &&
      input_width == average_pooling_op->last_input_width)
  {
    valid_batch_size = average_pooling_op->valid_batch_size;
    if (batch_size <= valid_batch_size) {
      average_pooling_op->compute.range[0] = batch_size;
      average_pooling_op->state = xnn_run_state_ready;
      return xnn_status_success;
    }
  }

  const size_t pooling_height = average_pooling_op->kernel_height;
  const size_t pooling_width = average_pooling_op->kernel_width;
  const size_t pooling_size = pooling_height * pooling_width;
  const size_t output_height = average_pooling_op->output_height;
  const size_t output_width = average_pooling_op->output_width;
  // Micro-kernel may read up to (mr - 1) elements after the end of indirection buffer.
  const uint32_t mr = xnn_params.f32.avgpool.mr;
  assert(mr == xnn_params.f32.pavgpool.mr);

  const size_t step_width = min(average_pooling_op->stride_width, pooling_width);
  const size_t step_height = pooling_size + (output_width * step_width - 1) * pooling_height;
  const size_t indirection_buffer_size = sizeof(void*) * ((mr - 1) + batch_size * output_height * step_height);

  const void** indirection_buffer = (const void**) realloc(average_pooling_op->indirection_buffer, indirection_buffer_size);
  if (indirection_buffer == NULL) {
    xnn_log_error("failed to allocate %zu bytes for indirection buffer", indirection_buffer_size);
    return xnn_status_out_of_memory;
  }
  average_pooling_op->indirection_buffer = indirection_buffer;

  xnn_indirection_init_dwconv2d(
    average_pooling_op, valid_batch_size, step_height, step_width, 2 /* log2(sizeof(float)) */);

  const size_t channels = average_pooling_op->channels;

  const size_t indirect_input_height_stride = step_height * sizeof(void*);
  const size_t output_width_stride = average_pooling_op->output_pixel_stride * sizeof(float);
  const size_t output_height_stride = output_width * output_width_stride;

  switch (average_pooling_op->ukernel.type) {
    case xnn_ukernel_type_average_pooling:
    {
      const uint32_t qr = xnn_params.f32.avgpool.qr;
      const size_t multipass_adjustment =
        pooling_size > mr ? round_up(pooling_size - mr, qr) + mr - qr : 0;
      average_pooling_op->context.average_pooling = (struct average_pooling_context) {
        .indirect_input = indirection_buffer,
        .indirect_input_batch_stride = output_height * indirect_input_height_stride,
        .indirect_input_height_stride = indirect_input_height_stride,
        .output = output,
        .output_batch_stride = output_height * output_height_stride,
        .output_height_stride = output_height_stride,
        .output_width = output_width,
        .pooling_size = pooling_size,
        .channels = channels,
        .zero = average_pooling_op->zero_buffer,
        .input_increment = (pooling_height * step_width - multipass_adjustment) * sizeof(void*),
        .output_increment = output_width_stride - channels * sizeof(float),
        .params.f32 = average_pooling_op->f32_avgpool_params,
      };
      if (pooling_size <= mr) {
        average_pooling_op->context.average_pooling.unipass_ukernel = xnn_params.f32.avgpool.up;
        average_pooling_op->compute.task_2d = (pthreadpool_task_2d_t) xnn_compute_average_pooling_unipass;
      } else {
        average_pooling_op->context.average_pooling.multipass_ukernel = xnn_params.f32.avgpool.mp;
        average_pooling_op->compute.task_2d = (pthreadpool_task_2d_t) xnn_compute_average_pooling_multipass;
      }
      break;
    }
    case xnn_ukernel_type_pixelwise_average_pooling:
    {
      if (input_height != average_pooling_op->last_input_height ||
          input_width != average_pooling_op->last_input_width)
      {
        const size_t pixelwise_buffer_size = output_height * output_width * sizeof(float);
        float* pixelwise_buffer = (float*) realloc(average_pooling_op->pixelwise_buffer, pixelwise_buffer_size);
        if (pixelwise_buffer == NULL) {
          xnn_log_error("failed to allocate %zu bytes for pixelwise buffer", pixelwise_buffer_size);
          return xnn_status_out_of_memory;
        }
        average_pooling_op->pixelwise_buffer = pixelwise_buffer;

        float* pixelwise_pointer = pixelwise_buffer;
        for (size_t output_y = 0; output_y < output_height; output_y++) {
          const size_t input_y_start = doz(output_y * average_pooling_op->stride_height, average_pooling_op->padding_top);
          const size_t input_y_end =
            min(doz(output_y * average_pooling_op->stride_height + average_pooling_op->kernel_height, average_pooling_op->padding_top), input_height);
          const uint32_t input_y_range = (uint32_t) (input_y_end - input_y_start);
          for (size_t output_x = 0; output_x < output_width; output_x++) {
            const size_t input_x_start = doz(output_x * average_pooling_op->stride_width, average_pooling_op->padding_left);
            const size_t input_x_end =
              min(doz(output_x * average_pooling_op->stride_width + average_pooling_op->kernel_width, average_pooling_op->padding_left), input_width);
            const uint32_t input_x_range = (uint32_t) (input_x_end - input_x_start);
            *pixelwise_pointer++ = 1.0f / ((float) (int32_t) (input_y_range * input_x_range));
          }
        }
      }

      const uint32_t qr = xnn_params.f32.pavgpool.qr;
      const size_t multipass_adjustment =
        pooling_size > mr ? round_up(pooling_size - mr, qr) + mr - qr : 0;
      average_pooling_op->context.pixelwise_average_pooling = (struct pixelwise_average_pooling_context) {
        .indirect_input = indirection_buffer,
        .indirect_input_batch_stride = output_height * indirect_input_height_stride,
        .indirect_input_height_stride = indirect_input_height_stride,
        .pixelwise_buffer = average_pooling_op->pixelwise_buffer,
        .pixelwise_buffer_height_stride = output_width * sizeof(float),
        .output = output,
        .output_batch_stride = output_height * output_height_stride,
        .output_height_stride = output_height_stride,
        .output_width = output_width,
        .pooling_size = pooling_size,
        .channels = channels,
        .zero = average_pooling_op->zero_buffer,
        .input_increment = (pooling_height * step_width - multipass_adjustment) * sizeof(void*),
        .output_increment = output_width_stride - channels * sizeof(float),
        .params.f32 = average_pooling_op->f32_output_params,
      };
      if (pooling_size <= mr) {
        average_pooling_op->context.pixelwise_average_pooling.unipass_ukernel = xnn_params.f32.pavgpool.up;
        average_pooling_op->compute.task_2d = (pthreadpool_task_2d_t) xnn_compute_pixelwise_average_pooling_unipass;
      } else {
        average_pooling_op->context.pixelwise_average_pooling.multipass_ukernel = xnn_params.f32.pavgpool.mp;
        average_pooling_op->compute.task_2d = (pthreadpool_task_2d_t) xnn_compute_pixelwise_average_pooling_multipass;
      }
      break;
    }
    default:
      XNN_UNREACHABLE;
  }
  average_pooling_op->compute.type = xnn_parallelization_type_2d;
  average_pooling_op->compute.range[0] = batch_size;
  average_pooling_op->compute.range[1] = output_height;
  average_pooling_op->state = xnn_run_state_ready;

  average_pooling_op->last_input = input;
  average_pooling_op->last_input_height = input_height;
  average_pooling_op->last_input_width = input_width;
  average_pooling_op->valid_batch_size = max(valid_batch_size, batch_size);

  return xnn_status_success;
}
