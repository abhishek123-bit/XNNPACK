// Copyright 2019 Google LLC
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree.

#include <assert.h>

#include <xnnpack/argmaxpool.h>
#include <xnnpack/math.h>


void xnn_f32_argmaxpool_ukernel_mp9p8q__scalar(
    size_t n,
    size_t ks,
    size_t kc,
    const float** input,
    float* acc_buffer,
    uint32_t* index_buffer,
    float* output,
    uint32_t* index,
    size_t input_increment,
    size_t output_increment,
    const union xnn_f32_output_params params[restrict static 1])
{
  assert(n != 0);
  assert(ks != 0);
  assert(ks > 9);
  assert(kc != 0);

  const float voutput_max = params->scalar.max;
  const float voutput_min = params->scalar.min;
  do {
    {
      float* ab = acc_buffer;
      uint32_t* ib = index_buffer;

      const float* i0 = *input++;
      const float* i1 = *input++;
      const float* i2 = *input++;
      const float* i3 = *input++;
      const float* i4 = *input++;
      const float* i5 = *input++;
      const float* i6 = *input++;
      const float* i7 = *input++;
      const float* i8 = *input++;

      size_t k = kc;
      do {
        const float vi0 = *i0++;
        const float vi1 = *i1++;
        const float vi2 = *i2++;
        const float vi3 = *i3++;
        const float vi4 = *i4++;
        const float vi5 = *i5++;
        const float vi6 = *i6++;
        const float vi7 = *i7++;
        const float vi8 = *i8++;

        float vmax = vi0;
        uint32_t vidx = 0;

        if (vi1 > vmax) {
          vmax = vi1;
          vidx = 1;
        }

        if (vi2 > vmax) {
          vmax = vi2;
          vidx = 2;
        }

        if (vi3 > vmax) {
          vmax = vi3;
          vidx = 3;
        }

        if (vi4 > vmax) {
          vmax = vi4;
          vidx = 4;
        }

        if (vi5 > vmax) {
          vmax = vi5;
          vidx = 5;
        }

        if (vi6 > vmax) {
          vmax = vi6;
          vidx = 6;
        }

        if (vi7 > vmax) {
          vmax = vi7;
          vidx = 7;
        }

        if (vi8 > vmax) {
          vmax = vi8;
          vidx = 8;
        }

        *ab++ = vmax;
        *ib++ = vidx;
      } while (--k != 0);
    }
    uint32_t vidx0 = 9;
    size_t m = ks;
    for (m -= 9; m > 8; m -= 8) {
      const float* i0 = *input++;
      const float* i1 = *input++;
      const float* i2 = *input++;
      const float* i3 = *input++;
      const float* i4 = *input++;
      const float* i5 = *input++;
      const float* i6 = *input++;
      const float* i7 = *input++;

      float* ab = acc_buffer;
      uint32_t* ib = index_buffer;

      size_t k = kc;
      do {
        const float vi0 = *i0++;
        const float vi1 = *i1++;
        const float vi2 = *i2++;
        const float vi3 = *i3++;
        const float vi4 = *i4++;
        const float vi5 = *i5++;
        const float vi6 = *i6++;
        const float vi7 = *i7++;

        float vmax = *ab;
        uint32_t vidx = *ib;

        if (vi0 > vmax) {
          vmax = vi0;
          vidx = vidx0;
        }

        if (vi1 > vmax) {
          vmax = vi1;
          vidx = vidx0 + 1;
        }

        if (vi2 > vmax) {
          vmax = vi2;
          vidx = vidx0 + 2;
        }

        if (vi3 > vmax) {
          vmax = vi3;
          vidx = vidx0 + 3;
        }

        if (vi4 > vmax) {
          vmax = vi4;
          vidx = vidx0 + 4;
        }

        if (vi5 > vmax) {
          vmax = vi5;
          vidx = vidx0 + 5;
        }

        if (vi6 > vmax) {
          vmax = vi6;
          vidx = vidx0 + 6;
        }

        if (vi7 > vmax) {
          vmax = vi7;
          vidx = vidx0 + 7;
        }

        *ab++ = vmax;
        *ib++ = vidx;
      } while (--k != 0);
      vidx0 += 8;
    }

    float* o = output;
    uint32_t* i = index;
    {
      const float* i0 = input[0];
      const float* i1 = input[1];
      const float* i2 = input[2];
      const float* i3 = input[3];
      const float* i4 = input[4];
      const float* i5 = input[5];
      const float* i6 = input[6];
      const float* i7 = input[7];
      input = (const float**) ((uintptr_t) input + input_increment);
      if (m < 2) {
        i1 = i0;
      }
      if (m <= 2) {
        i2 = i0;
      }
      if (m < 4) {
        i3 = i0;
      }
      if (m <= 4) {
        i4 = i0;
      }
      if (m < 6) {
        i5 = i0;
      }
      if (m <= 6) {
        i6 = i0;
      }
      if (m != 8) {
        i7 = i0;
      }

      size_t k = kc;
      float* ab = acc_buffer;
      uint32_t* ib = index_buffer;
      do {
        const float vi0 = *i0++;
        const float vi1 = *i1++;
        const float vi2 = *i2++;
        const float vi3 = *i3++;
        const float vi4 = *i4++;
        const float vi5 = *i5++;
        const float vi6 = *i6++;
        const float vi7 = *i7++;

        float vmax = *ab++;
        uint32_t vidx = *ib++;

        if (vi0 > vmax) {
          vmax = vi0;
          vidx = vidx0;
        }

        if (vi1 > vmax) {
          vmax = vi1;
          vidx = vidx0 + 1;
        }

        if (vi2 > vmax) {
          vmax = vi2;
          vidx = vidx0 + 2;
        }

        if (vi3 > vmax) {
          vmax = vi3;
          vidx = vidx0 + 3;
        }

        if (vi4 > vmax) {
          vmax = vi4;
          vidx = vidx0 + 4;
        }

        if (vi5 > vmax) {
          vmax = vi5;
          vidx = vidx0 + 5;
        }

        if (vi6 > vmax) {
          vmax = vi6;
          vidx = vidx0 + 6;
        }

        if (vi7 > vmax) {
          vmax = vi7;
          vidx = vidx0 + 7;
        }

        const float vout = math_max_f32(math_min_f32(vmax, voutput_max), voutput_min);

        *o++ = vout;
        *i++ = vidx;
      } while (--k != 0);
    }

    output = (float*) ((uintptr_t) o + output_increment);
    index = (uint32_t*) i;
  } while (--n != 0);
}
