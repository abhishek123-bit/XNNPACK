// Copyright 2019 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include <gtest/gtest.h>

#include "argmax-pooling-operator-tester.h"

#include <xnnpack/params.h>


static uint32_t FindMaxSinglePassPoolingSize(const argmaxpool_parameters* ukernel) {
  uint32_t mr = 0;
  while (ukernel->qr == 0) {
    mr = std::max<uint32_t>(mr, ukernel->mr);
    ukernel++;
  }
  return mr;
}

static argmaxpool_parameters FindMultiPassMicroKernel(const argmaxpool_parameters* ukernel) {
  while (ukernel->qr == 0) {
    ukernel++;
  }
  return *ukernel;
}

TEST(ARGMAX_POOLING_OP_F32, unit_batch_small_1xM_pool) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = 2; pool_size <= FindMaxSinglePassPoolingSize(xnn_params.f32.argmaxpool); pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(2)
        .input_width(pool_size + 2)
        .pooling_height(1)
        .pooling_width(pool_size)
        .channels(channels)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, unit_batch_small_1xM_pool_with_padding) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = 3; pool_size <= FindMaxSinglePassPoolingSize(xnn_params.f32.argmaxpool); pool_size++) {
      for (size_t padding_left = 0; padding_left <= 1; padding_left++) {
        for (size_t padding_right = 0; padding_right <= 1; padding_right++) {
          ArgmaxPoolingOperatorTester()
            .batch_size(1)
            .input_height(2)
            .input_width(pool_size + 2)
            .padding_left(padding_left)
            .padding_right(padding_right)
            .pooling_height(1)
            .pooling_width(pool_size)
            .channels(channels)
            .TestF32();
        }
      }
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, unit_batch_small_Mx1_pool) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = 2; pool_size <= FindMaxSinglePassPoolingSize(xnn_params.f32.argmaxpool); pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(pool_size + 1)
        .input_width(3)
        .pooling_height(pool_size)
        .pooling_width(1)
        .channels(channels)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, unit_batch_small_Mx1_pool_with_padding) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = 2; pool_size <= FindMaxSinglePassPoolingSize(xnn_params.f32.argmaxpool); pool_size++) {
      for (size_t padding_top = 0; padding_top <= 1; padding_top++) {
        for (size_t padding_bottom = 0; padding_bottom <= 1; padding_bottom++) {
          ArgmaxPoolingOperatorTester()
            .batch_size(1)
            .input_height(pool_size + 1)
            .input_width(3)
            .padding_top(padding_top)
            .padding_bottom(padding_bottom)
            .pooling_height(pool_size)
            .pooling_width(1)
            .channels(channels)
            .TestF32();
        }
      }
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, unit_batch_small_pool_with_input_stride) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = 2; pool_size <= FindMaxSinglePassPoolingSize(xnn_params.f32.argmaxpool); pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(pool_size + 1)
        .input_width(3)
        .pooling_height(pool_size)
        .pooling_width(1)
        .channels(channels)
        .input_pixel_stride(5 * channels)
        .TestF32();
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(2)
        .input_width(pool_size + 2)
        .pooling_height(1)
        .pooling_width(pool_size)
        .channels(channels)
        .input_pixel_stride(5 * channels)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, unit_batch_small_pool_with_output_stride) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = 2; pool_size <= FindMaxSinglePassPoolingSize(xnn_params.f32.argmaxpool); pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(pool_size + 1)
        .input_width(3)
        .pooling_height(pool_size)
        .pooling_width(1)
        .channels(channels)
        .output_pixel_stride(5 * channels)
        .TestF32();
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(2)
        .input_width(pool_size + 2)
        .pooling_height(1)
        .pooling_width(pool_size)
        .channels(channels)
        .output_pixel_stride(5 * channels)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, unit_batch_small_pool_with_qmin) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = 2; pool_size <= FindMaxSinglePassPoolingSize(xnn_params.f32.argmaxpool); pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(pool_size + 1)
        .input_width(3)
        .pooling_height(pool_size)
        .pooling_width(1)
        .channels(channels)
        .qmin(192)
        .TestF32();
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(2)
        .input_width(pool_size + 2)
        .pooling_height(1)
        .pooling_width(pool_size)
        .channels(channels)
        .qmin(192)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, unit_batch_small_pool_with_qmax) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = 2; pool_size <= FindMaxSinglePassPoolingSize(xnn_params.f32.argmaxpool); pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(pool_size + 1)
        .input_width(3)
        .pooling_height(pool_size)
        .pooling_width(1)
        .channels(channels)
        .qmax(192)
        .TestF32();
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(2)
        .input_width(pool_size + 2)
        .pooling_height(1)
        .pooling_width(pool_size)
        .channels(channels)
        .qmax(192)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, unit_batch_large_1xM_pool) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  const auto multipass = FindMultiPassMicroKernel(xnn_params.f32.argmaxpool);
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = multipass.mr + 1; pool_size <= multipass.mr + multipass.qr; pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(2)
        .input_width(pool_size + 2)
        .pooling_height(1)
        .pooling_width(pool_size)
        .channels(channels)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, unit_batch_large_1xM_pool_with_padding) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  const auto multipass = FindMultiPassMicroKernel(xnn_params.f32.argmaxpool);
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = multipass.mr + 1; pool_size <= multipass.mr + multipass.qr; pool_size++) {
      for (size_t padding_left = 0; padding_left <= 1; padding_left++) {
        for (size_t padding_right = 0; padding_right <= 1; padding_right++) {
          ArgmaxPoolingOperatorTester()
            .batch_size(1)
            .input_height(2)
            .input_width(pool_size + 2)
            .padding_left(padding_left)
            .padding_right(padding_right)
            .pooling_height(1)
            .pooling_width(pool_size)
            .channels(channels)
            .TestF32();
        }
      }
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, unit_batch_large_Mx1_pool) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  const auto multipass = FindMultiPassMicroKernel(xnn_params.f32.argmaxpool);
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = multipass.mr + 1; pool_size <= multipass.mr + multipass.qr; pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(pool_size + 1)
        .input_width(3)
        .pooling_height(pool_size)
        .pooling_width(1)
        .channels(channels)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, unit_batch_large_Mx1_pool_with_padding) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  const auto multipass = FindMultiPassMicroKernel(xnn_params.f32.argmaxpool);
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = multipass.mr + 1; pool_size <= multipass.mr + multipass.qr; pool_size++) {
      for (size_t padding_top = 0; padding_top <= 1; padding_top++) {
        for (size_t padding_bottom = 0; padding_bottom <= 1; padding_bottom++) {
          ArgmaxPoolingOperatorTester()
            .batch_size(1)
            .input_height(pool_size + 1)
            .input_width(3)
            .padding_top(padding_top)
            .padding_bottom(padding_bottom)
            .pooling_height(pool_size)
            .pooling_width(1)
            .channels(channels)
            .TestF32();
        }
      }
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, unit_batch_large_pool_with_input_stride) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  const auto multipass = FindMultiPassMicroKernel(xnn_params.f32.argmaxpool);
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = multipass.mr + 1; pool_size <= multipass.mr + multipass.qr; pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(pool_size + 1)
        .input_width(3)
        .pooling_height(pool_size)
        .pooling_width(1)
        .channels(channels)
        .input_pixel_stride(5 * channels)
        .TestF32();
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(2)
        .input_width(pool_size + 2)
        .pooling_height(1)
        .pooling_width(pool_size)
        .channels(channels)
        .input_pixel_stride(5 * channels)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, unit_batch_large_pool_with_output_stride) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  const auto multipass = FindMultiPassMicroKernel(xnn_params.f32.argmaxpool);
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = multipass.mr + 1; pool_size <= multipass.mr + multipass.qr; pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(pool_size + 1)
        .input_width(3)
        .pooling_height(pool_size)
        .pooling_width(1)
        .channels(channels)
        .output_pixel_stride(5 * channels)
        .TestF32();
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(2)
        .input_width(pool_size + 2)
        .pooling_height(1)
        .pooling_width(pool_size)
        .channels(channels)
        .output_pixel_stride(5 * channels)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, unit_batch_large_pool_with_qmin) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  const auto multipass = FindMultiPassMicroKernel(xnn_params.f32.argmaxpool);
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = multipass.mr + 1; pool_size <= multipass.mr + multipass.qr; pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(pool_size + 1)
        .input_width(3)
        .pooling_height(pool_size)
        .pooling_width(1)
        .channels(channels)
        .qmin(192)
        .TestF32();
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(2)
        .input_width(pool_size + 2)
        .pooling_height(1)
        .pooling_width(pool_size)
        .channels(channels)
        .qmin(192)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, unit_batch_large_pool_with_qmax) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  const auto multipass = FindMultiPassMicroKernel(xnn_params.f32.argmaxpool);
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = multipass.mr + 1; pool_size <= multipass.mr + multipass.qr; pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(pool_size + 1)
        .input_width(3)
        .pooling_height(pool_size)
        .pooling_width(1)
        .channels(channels)
        .qmax(192)
        .TestF32();
      ArgmaxPoolingOperatorTester()
        .batch_size(1)
        .input_height(2)
        .input_width(pool_size + 2)
        .pooling_height(1)
        .pooling_width(pool_size)
        .channels(channels)
        .qmax(192)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, small_batch_small_pool) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = 2; pool_size <= FindMaxSinglePassPoolingSize(xnn_params.f32.argmaxpool); pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(3)
        .input_height(pool_size + 1)
        .input_width(3)
        .pooling_height(pool_size)
        .pooling_width(1)
        .channels(channels)
        .TestF32();
      ArgmaxPoolingOperatorTester()
        .batch_size(3)
        .input_height(2)
        .input_width(pool_size + 2)
        .pooling_height(1)
        .pooling_width(pool_size)
        .channels(channels)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, small_batch_small_pool_with_input_stride) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = 2; pool_size <= FindMaxSinglePassPoolingSize(xnn_params.f32.argmaxpool); pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(3)
        .input_height(pool_size + 1)
        .input_width(3)
        .pooling_height(pool_size)
        .pooling_width(1)
        .channels(channels)
        .input_pixel_stride(5 * channels)
        .TestF32();
      ArgmaxPoolingOperatorTester()
        .batch_size(3)
        .input_height(2)
        .input_width(pool_size + 2)
        .pooling_height(1)
        .pooling_width(pool_size)
        .channels(channels)
        .input_pixel_stride(5 * channels)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, small_batch_small_pool_with_output_stride) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = 2; pool_size <= FindMaxSinglePassPoolingSize(xnn_params.f32.argmaxpool); pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(3)
        .input_height(pool_size + 1)
        .input_width(3)
        .pooling_height(pool_size)
        .pooling_width(1)
        .channels(channels)
        .output_pixel_stride(5 * channels)
        .TestF32();
      ArgmaxPoolingOperatorTester()
        .batch_size(3)
        .input_height(2)
        .input_width(pool_size + 2)
        .pooling_height(1)
        .pooling_width(pool_size)
        .channels(channels)
        .output_pixel_stride(5 * channels)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, small_batch_large_pool) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  const auto multipass = FindMultiPassMicroKernel(xnn_params.f32.argmaxpool);
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = multipass.mr + 1; pool_size <= multipass.mr + multipass.qr; pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(3)
        .input_height(pool_size + 1)
        .input_width(3)
        .pooling_height(pool_size)
        .pooling_width(1)
        .channels(channels)
        .TestF32();
      ArgmaxPoolingOperatorTester()
        .batch_size(3)
        .input_height(2)
        .input_width(pool_size + 2)
        .pooling_height(1)
        .pooling_width(pool_size)
        .channels(channels)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, small_batch_large_pool_with_input_stride) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  const auto multipass = FindMultiPassMicroKernel(xnn_params.f32.argmaxpool);
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = multipass.mr + 1; pool_size <= multipass.mr + multipass.qr; pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(3)
        .input_height(pool_size + 1)
        .input_width(3)
        .pooling_height(pool_size)
        .pooling_width(1)
        .channels(channels)
        .input_pixel_stride(5 * channels)
        .TestF32();
      ArgmaxPoolingOperatorTester()
        .batch_size(3)
        .input_height(2)
        .input_width(pool_size + 2)
        .pooling_height(1)
        .pooling_width(pool_size)
        .channels(channels)
        .input_pixel_stride(5 * channels)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, small_batch_large_pool_with_output_stride) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  const auto multipass = FindMultiPassMicroKernel(xnn_params.f32.argmaxpool);
  for (size_t channels = 1; channels <= 100; channels += 15) {
    for (size_t pool_size = multipass.mr + 1; pool_size <= multipass.mr + multipass.qr; pool_size++) {
      ArgmaxPoolingOperatorTester()
        .batch_size(3)
        .input_height(pool_size + 1)
        .input_width(3)
        .pooling_height(pool_size)
        .pooling_width(1)
        .channels(channels)
        .output_pixel_stride(5 * channels)
        .TestF32();
      ArgmaxPoolingOperatorTester()
        .batch_size(3)
        .input_height(2)
        .input_width(pool_size + 2)
        .pooling_height(1)
        .pooling_width(pool_size)
        .channels(channels)
        .output_pixel_stride(5 * channels)
        .TestF32();
    }
  }
}

TEST(ARGMAX_POOLING_OP_F32, setup_increasing_batch) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  ArgmaxPoolingOperatorTester()
    .batch_size(3)
    .next_batch_size(5)
    .input_height(8)
    .input_width(8)
    .pooling_height(5)
    .pooling_width(3)
    .channels(24)
    .TestSetupF32();
}

TEST(ARGMAX_POOLING_OP_F32, setup_decreasing_batch) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  ArgmaxPoolingOperatorTester()
    .batch_size(5)
    .next_batch_size(3)
    .input_height(8)
    .input_width(8)
    .pooling_height(5)
    .pooling_width(3)
    .channels(24)
    .TestSetupF32();
}

TEST(ARGMAX_POOLING_OP_F32, setup_changing_height) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  ArgmaxPoolingOperatorTester()
    .batch_size(3)
    .input_height(8)
    .input_width(8)
    .next_input_height(9)
    .pooling_height(5)
    .pooling_width(3)
    .channels(24)
    .TestSetupF32();
  ArgmaxPoolingOperatorTester()
    .batch_size(3)
    .input_height(8)
    .input_width(8)
    .next_input_height(7)
    .pooling_height(5)
    .pooling_width(3)
    .channels(24)
    .TestSetupF32();
}

TEST(ARGMAX_POOLING_OP_F32, setup_changing_width) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  ArgmaxPoolingOperatorTester()
    .batch_size(3)
    .input_height(8)
    .input_width(8)
    .next_input_width(9)
    .pooling_height(5)
    .pooling_width(3)
    .channels(24)
    .TestSetupF32();
  ArgmaxPoolingOperatorTester()
    .batch_size(3)
    .input_height(8)
    .input_width(8)
    .next_input_width(7)
    .pooling_height(5)
    .pooling_width(3)
    .channels(24)
    .TestSetupF32();
}

TEST(ARGMAX_POOLING_OP_F32, setup_swap_height_and_width) {
  ASSERT_EQ(xnn_status_success, xnn_initialize());
  ArgmaxPoolingOperatorTester()
    .batch_size(3)
    .input_height(9)
    .input_width(8)
    .next_input_height(8)
    .next_input_width(9)
    .pooling_height(5)
    .pooling_width(3)
    .channels(24)
    .TestSetupF32();
}
