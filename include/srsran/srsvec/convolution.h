/*
 *
 * Copyright 2021-2023 Software Radio Systems Limited
 *
 * By using this file, you agree to the terms and conditions set
 * forth in the LICENSE file which can be found at the top level of
 * the distribution.
 *
 */

/// \file
/// \brief Convolution declarations.

#pragma once

#include "zero.h"
#include "srsran/srsvec/detail/traits.h"
#include "srsran/srsvec/types.h"
#include <numeric>

namespace srsran {
namespace srsvec {

namespace detail {

/// Check if two span-compatibles are the different: always true if they are of different type.
template <typename T, typename U>
bool check_different(const T& /* na */, const U& /* na */)
{
  return true;
}

/// Check if two span-compatibles are the different: if they are of the same type, check the data pointers.
template <typename T>
bool check_different(const T& in1, const T& in2)
{
  return (in1.data() != in2.data());
}

void multiplicate_and_accumulate(span<float> out_chunk, span<const float> x_chunk, span<const float> y);
void multiplicate_and_accumulate(span<cf_t> out_chunk, span<const float> x_chunk, span<const cf_t> y);
void multiplicate_and_accumulate(span<cf_t> out_chunk, span<const cf_t> x_chunk, span<const float> y);

} // namespace detail

/// \brief Convolution between two sequences.
///
/// Computes the convolution between two sequences \f$x\f$ and \f$y\f$, namely \f$z[n] = \sum_{i} x[i] y[n-i]\f$ and
/// returns its central part, of the \e same size as the first input sequence.
/// \tparam V   Output type (must be compatible with a span of arithmetic/complex values).
/// \tparam T   Type of the first input sequence (must be compatible with a span of arithmetic/complex values).
/// \tparam U   Type of the second input sequence (must be compatible with a span of arithmetic/complex values).
/// \param[out] out  Output sequence - convolution between the input sequences.
/// \param[in]  x_v  First input sequence.
/// \param[in]  y_v  Second input sequence.
/// \remark An assertion is raised if the length of the second sequence is larger than the length of the first one.
/// \remark This implementation of the convolution is not based on FFT and should only be used when the second input
/// sequence is "very" short.
template <typename V, typename T, typename U>
void convolution_same(V&& out, const T& x_v, const U& y_v)
{
  static_assert((detail::is_arithmetic_span_compatible<T>::value || detail::is_complex_span_compatible<T>::value),
                "Template type is not compatible with a span of arithmetics or complex floats.");
  static_assert((detail::is_arithmetic_span_compatible<U>::value || detail::is_complex_span_compatible<U>::value),
                "Template type is not compatible with a span of arithmetics or complex floats.");
  static_assert((detail::is_arithmetic_span_compatible<V>::value || detail::is_complex_span_compatible<V>::value),
                "Template type is not compatible with a span of arithmetics or complex floats.");
  srsran_srsvec_assert_size(out, x_v);

  unsigned x_size = x_v.size();
  unsigned y_size = y_v.size();
  srsran_assert((y_size <= x_size),
                "The current implementation of the convolution is only defined when the second input is not longer "
                "than the first one.");

  using Type_y   = typename U::value_type;
  using Type_x   = typename T::value_type;
  using Type_out = typename std::remove_reference<V>::type::value_type;

  srsran_assert(detail::check_different(out, x_v), "Cannot override input with output.");

  srsvec::zero(out);
  span<const Type_x> x(x_v);
  span<const Type_y> y(y_v);
  Type_out           init = 0;

  // At the beginning, x and y do not fully overlap.
  unsigned y_mid   = y_size / 2;
  unsigned i_out   = 0;
  unsigned out_end = x.size() - y_mid;
  for (unsigned n_els = y_mid + 1; n_els != y_size; ++n_els) {
    span<const Type_y> y_local = y.first(n_els);
    span<const Type_x> x_local = x.first(n_els);
    // Note that y_local is reversed.
    out[i_out++] = std::inner_product(x_local.begin(), x_local.end(), y_local.rbegin(), init);
  }
  // In the central part, y fully overlaps with x.
  detail::multiplicate_and_accumulate(out, x, y);

  // For the final part, we again take into account the partial overlap.
  unsigned start = i_out;
  for (unsigned n_els = x_size - out_end + start; n_els != start; --n_els) {
    span<const Type_y> y_local = y.last(n_els);
    span<const Type_x> x_local = x.last(n_els);
    // Note that y_local is reversed.
    out[out_end++] = std::inner_product(x_local.begin(), x_local.end(), y_local.rbegin(), init);
  }
}

} // namespace srsvec
} // namespace srsran