#ifndef REG_SAD_POW2_WIDTHS_SSE41_H_
#define REG_SAD_POW2_WIDTHS_SSE41_H_

#include <immintrin.h>
#include "kvazaar.h"

static uint32_t reg_sad_w8(const kvz_pixel * const data1, const kvz_pixel * const data2,
                           const int32_t height, const uint32_t stride1,
                           const uint32_t stride2)
{
  __m128i sse_inc = _mm_setzero_si128();
  uint64_t result = 0;
  int32_t y;

  const int32_t height_xmm_bytes = height & ~1;
  const int32_t height_parity    = height &  1;

  for (y = 0; y < height_xmm_bytes; y += 2) {
    __m128d a_d = _mm_setzero_pd();
    __m128d b_d = _mm_setzero_pd();

    a_d = _mm_loadl_pd(a_d, (const double *)(data1 + (y + 0) * stride1));
    b_d = _mm_loadl_pd(b_d, (const double *)(data2 + (y + 0) * stride2));
    a_d = _mm_loadh_pd(a_d, (const double *)(data1 + (y + 1) * stride1));
    b_d = _mm_loadh_pd(b_d, (const double *)(data2 + (y + 1) * stride2));

    __m128i a = _mm_castpd_si128(a_d);
    __m128i b = _mm_castpd_si128(b_d);

    __m128i curr_sads = _mm_sad_epu8(a, b);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads);
  }
  if (height_parity) {
    __m64 a = *(__m64 *)(data1 + y * stride1);
    __m64 b = *(__m64 *)(data2 + y * stride2);
    __m64 sads = _mm_sad_pu8(a, b);
    result = (uint64_t)sads;
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);

  result += _mm_cvtsi128_si32(sad);
  return result;
}

static uint32_t reg_sad_w16(const kvz_pixel * const data1, const kvz_pixel * const data2,
                            const int32_t height, const uint32_t stride1,
                            const uint32_t stride2)
{
  __m128i sse_inc = _mm_setzero_si128();
  int32_t y;
  for (y = 0; y < height; y++) {
    __m128i a = _mm_loadu_si128((__m128i const*) &data1[y * stride1]);
    __m128i b = _mm_loadu_si128((__m128i const*) &data2[y * stride2]);
    __m128i curr_sads = _mm_sad_epu8(a, b);
    sse_inc = _mm_add_epi64(sse_inc, curr_sads);
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);
  return _mm_cvtsi128_si32(sad);
}

static uint32_t reg_sad_arbitrary(const kvz_pixel * const data1, const kvz_pixel * const data2,
                                  const int32_t width, const int32_t height, const uint32_t stride1,
                                  const uint32_t stride2)
{
  int32_t y, x;
  __m128i sse_inc = _mm_setzero_si128();
  
  // Bytes in block in 128-bit blocks per each scanline, and remainder
  const int32_t largeblock_bytes = width & ~15;
  const int32_t residual_bytes   = width &  15;

  const __m128i rds    = _mm_set1_epi8 (residual_bytes);
  const __m128i ns     = _mm_setr_epi8 (0,  1,  2,  3,  4,  5,  6,  7,
                                        8,  9,  10, 11, 12, 13, 14, 15);
  const __m128i rdmask = _mm_cmpgt_epi8(rds, ns);

  for (y = 0; y < height; ++y) {
    for (x = 0; x < largeblock_bytes; x += 16) {
      __m128i a = _mm_loadu_si128((__m128i const*) &data1[y * stride1 + x]);
      __m128i b = _mm_loadu_si128((__m128i const*) &data2[y * stride2 + x]);
      __m128i curr_sads = _mm_sad_epu8(a, b);
      sse_inc = _mm_add_epi32(sse_inc, curr_sads);
    }
    
    if (residual_bytes) {
      __m128i a = _mm_loadu_si128((__m128i const*) &data1[y * stride1 + x]);
      __m128i b = _mm_loadu_si128((__m128i const*) &data2[y * stride2 + x]);

      __m128i b_masked  = _mm_blendv_epi8(a, b, rdmask);
      __m128i curr_sads = _mm_sad_epu8(a, b_masked);
      sse_inc = _mm_add_epi32(sse_inc, curr_sads);
    }
  }
  __m128i sse_inc_2 = _mm_shuffle_epi32(sse_inc, _MM_SHUFFLE(1, 0, 3, 2));
  __m128i sad       = _mm_add_epi64    (sse_inc, sse_inc_2);

  return _mm_cvtsi128_si32(sad);
}



#endif
