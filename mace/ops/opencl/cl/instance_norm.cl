#include <common.h>
// Supported data types: half/float
__kernel void instance_norm(OUT_OF_RANGE_PARAMS
                            GLOBAL_WORK_GROUP_SIZE_DIM3
                            __read_only image2d_t input,
#ifdef IN_AFFINE
                            __read_only image2d_t scale,
                            __read_only image2d_t offset,
#endif
                            __read_only image2d_t mean,
                            __read_only image2d_t var,
                            __private const float epsilon,
                            __write_only image2d_t output,
                            __private const float relux_max_limit,
                            __private const float activation_coefficient,
                            __private const int height) {
  const int ch_blk = get_global_id(0);
  const int w = get_global_id(1);
  const int hb = get_global_id(2);
  const int batch_idx = hb / height;

#ifndef NON_UNIFORM_WORK_GROUP
  if (ch_blk >= global_size_dim0 || w >= global_size_dim1
      || hb >= global_size_dim2) {
    return;
  }
#endif

  DATA_TYPE4 mean_value = READ_IMAGET(mean, SAMPLER, (int2)(ch_blk, batch_idx));
  DATA_TYPE4 var_value = READ_IMAGET(var, SAMPLER, (int2)(ch_blk, batch_idx));
#ifdef IN_AFFINE
  DATA_TYPE4 scale_value = READ_IMAGET(scale, SAMPLER, (int2)(ch_blk, 0));
  DATA_TYPE4 offset_value = READ_IMAGET(offset, SAMPLER, (int2)(ch_blk, 0));

  DATA_TYPE4 bn_scale = scale_value * rsqrt(var_value + (DATA_TYPE4)epsilon);
  DATA_TYPE4 bn_offset = mad(0 - mean_value, bn_scale, offset_value);
#endif

  const int width = global_size_dim1;
  const int pos = mad24(ch_blk, width, w);
  DATA_TYPE4 in = READ_IMAGET(input, SAMPLER, (int2)(pos, hb));
#ifdef IN_AFFINE
  DATA_TYPE4 out = mad(in, bn_scale, bn_offset);
#else
  DATA_TYPE4 out = (in -  mean_value) * rsqrt(var_value + (DATA_TYPE4)epsilon);
#endif


#if  defined(USE_RELU) || defined(USE_LEAKYRELU) || defined(USE_RELUX) || defined(USE_TANH) || defined(USE_SIGMOID) || defined(USE_ELU)
  out = do_activation(out, relux_max_limit, activation_coefficient);
#endif

  WRITE_IMAGET(output, (int2)(pos, hb), out);
}
