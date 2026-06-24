// Copyright 2025-present the zvec project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <zvec/export.h>

namespace zvec {
namespace ailego {

/*! Float Helper
 */
struct ZVEC_AILEGO_API FloatHelper {
  //! Convert FP16 to FP32
  static float ToFP32(uint16_t val);

  //! Convert FP16 to FP32 (array)
  static void ToFP32(const uint16_t *arr, size_t size, float *out);

  //! Convert FP16 to FP32 with normalization (array)
  static void ToFP32(const uint16_t *arr, size_t size, float norm, float *out);

  //! Convert FP32 to FP16
  static uint16_t ToFP16(float val);

  //! Convert FP32 to FP16 (array)
  static void ToFP16(const float *arr, size_t size, uint16_t *out);

  //! Convert FP32 to FP16 with normalization (array)
  static void ToFP16(const float *arr, size_t size, float norm, uint16_t *out);

  //! Convert FP16 to FP32 with normalization
  static inline float ToFP32(uint16_t val, float norm) {
    return (FloatHelper::ToFP32(val) / norm);
  }

  //! Convert FP32 to FP16 with normalization
  static inline uint16_t ToFP16(float val, float norm) {
    return FloatHelper::ToFP16(val / norm);
  }
};

#if !defined(__aarch64__)
namespace detail {

inline uint16_t Float32ToFloat16Scalar(float val) {
  uint32_t bits;
  std::memcpy(&bits, &val, sizeof(bits));

  const uint16_t sign = static_cast<uint16_t>((bits >> 16) & 0x8000U);
  const uint32_t mantissa = bits & 0x007FFFFFU;
  const int32_t exponent = static_cast<int32_t>((bits >> 23) & 0xFFU) - 127;

  if (exponent == 128) {
    return static_cast<uint16_t>(sign | (mantissa ? 0x7E00U : 0x7C00U));
  }
  if (exponent > 15) {
    return static_cast<uint16_t>(sign | 0x7C00U);
  }
  if (exponent < -24) {
    return sign;
  }

  if (exponent < -14) {
    const uint32_t full_mantissa = mantissa | 0x00800000U;
    const int32_t shift = -exponent - 1;
    const uint32_t rounded = (full_mantissa + (1U << (shift - 1))) >> shift;
    return static_cast<uint16_t>(sign | rounded);
  }

  uint32_t rounded_mantissa = mantissa + 0x00001000U;
  uint32_t half_exponent = static_cast<uint32_t>(exponent + 15) << 10;
  if (rounded_mantissa & 0x00800000U) {
    rounded_mantissa = 0;
    half_exponent += 0x0400U;
    if (half_exponent >= 0x7C00U) {
      return static_cast<uint16_t>(sign | 0x7C00U);
    }
  }

  return static_cast<uint16_t>(sign | half_exponent | (rounded_mantissa >> 13));
}

inline float Float16ToFloat32Scalar(uint16_t val) {
  const uint32_t sign = static_cast<uint32_t>(val & 0x8000U) << 16;
  uint32_t exponent = (val >> 10) & 0x1FU;
  uint32_t mantissa = val & 0x03FFU;
  uint32_t bits = 0;

  if (exponent == 0) {
    if (mantissa == 0) {
      bits = sign;
    } else {
      int32_t normalized_exponent = -14;
      while ((mantissa & 0x0400U) == 0) {
        mantissa <<= 1;
        --normalized_exponent;
      }
      mantissa &= 0x03FFU;
      bits = sign | (static_cast<uint32_t>(normalized_exponent + 127) << 23) |
             (mantissa << 13);
    }
  } else if (exponent == 0x1FU) {
    bits = sign | 0x7F800000U | (mantissa << 13);
  } else {
    exponent = exponent + (127 - 15);
    bits = sign | (exponent << 23) | (mantissa << 13);
  }

  float result;
  std::memcpy(&result, &bits, sizeof(result));
  return result;
}

}  // namespace detail

/*! Half-Precision Floating Point
 */
class ZVEC_AILEGO_API Float16 {
 public:
  //! Constructor
  Float16(void) : value_(0) {}

  //! Constructor
  Float16(float val) : value_(detail::Float32ToFloat16Scalar(val)) {}

  //! Constructor
  Float16(double val)
      : value_(detail::Float32ToFloat16Scalar(static_cast<float>(val))) {}

  //! Assigment
  Float16 &operator=(float val) {
    this->value_ = detail::Float32ToFloat16Scalar(val);
    return *this;
  }

  //! Assigment
  Float16 &operator+=(float val) {
    this->value_ = detail::Float32ToFloat16Scalar(
        detail::Float16ToFloat32Scalar(this->value_) + val);
    return *this;
  }

  //! Assigment
  Float16 &operator-=(float val) {
    this->value_ = detail::Float32ToFloat16Scalar(
        detail::Float16ToFloat32Scalar(this->value_) - val);
    return *this;
  }

  //! Assigment
  Float16 &operator*=(float val) {
    this->value_ = detail::Float32ToFloat16Scalar(
        detail::Float16ToFloat32Scalar(this->value_) * val);
    return *this;
  }

  //! Assigment
  Float16 &operator/=(float val) {
    this->value_ = detail::Float32ToFloat16Scalar(
        detail::Float16ToFloat32Scalar(this->value_) / val);
    return *this;
  }

  //! Retrieve value in FP32
  operator float() const {
    return detail::Float16ToFloat32Scalar(this->value_);
  }

  //! Equal operator
  bool operator==(const Float16 &rhs) const {
    return this->value_ == rhs.value_;
  }

  //! No equal operator
  bool operator!=(const Float16 &rhs) const {
    return this->value_ != rhs.value_;
  }

  //! Less than operator
  bool operator<(const Float16 &rhs) const {
    return detail::Float16ToFloat32Scalar(this->value_) <
           detail::Float16ToFloat32Scalar(rhs.value_);
  }

  //! Less than or equal operator
  bool operator<=(const Float16 &rhs) const {
    return detail::Float16ToFloat32Scalar(this->value_) <=
           detail::Float16ToFloat32Scalar(rhs.value_);
  }

  //! Greater than operator
  bool operator>(const Float16 &rhs) const {
    return detail::Float16ToFloat32Scalar(this->value_) >
           detail::Float16ToFloat32Scalar(rhs.value_);
  }

  //! Greater than or equal operator
  bool operator>=(const Float16 &rhs) const {
    return detail::Float16ToFloat32Scalar(this->value_) >=
           detail::Float16ToFloat32Scalar(rhs.value_);
  }

  //! Calculate the absolute value
  static inline Float16 Absolute(const Float16 &x) {
    Float16 abs;
    abs.value_ = static_cast<uint16_t>(x.value_ & 0x7fff);
    return abs;
  }

 private:
  uint16_t value_;
};
#else
/*! Half-Precision Floating Point
 */
class ZVEC_AILEGO_API Float16 {
 public:
  //! Constructor
  Float16(void) : value_(0) {}

  //! Constructor
  Float16(__fp16 val) : value_(val) {}

  //! Assigment
  Float16 &operator=(__fp16 val) {
    this->value_ = val;
    return *this;
  }

  //! Assigment
  Float16 &operator+=(__fp16 val) {
    this->value_ = this->value_ + val;
    return *this;
  }

  //! Assigment
  Float16 &operator-=(__fp16 val) {
    this->value_ = this->value_ - val;
    return *this;
  }

  //! Assigment
  Float16 &operator*=(__fp16 val) {
    this->value_ = this->value_ * val;
    return *this;
  }

  //! Assigment
  Float16 &operator/=(__fp16 val) {
    this->value_ = this->value_ / val;
    return *this;
  }

  //! Retrieve value in FP16
  operator __fp16() const {
    return this->value_;
  }

  //! Equal operator
  bool operator==(const Float16 &rhs) const {
    return this->value_ == rhs.value_;
  }

  //! No equal operator
  bool operator!=(const Float16 &rhs) const {
    return this->value_ != rhs.value_;
  }

  //! Less than operator
  bool operator<(const Float16 &rhs) const {
    return this->value_ < rhs.value_;
  }

  //! Less than or equal operator
  bool operator<=(const Float16 &rhs) const {
    return this->value_ <= rhs.value_;
  }

  //! Greater than operator
  bool operator>(const Float16 &rhs) const {
    return this->value_ > rhs.value_;
  }

  //! Greater than or equal operator
  bool operator>=(const Float16 &rhs) const {
    return this->value_ >= rhs.value_;
  }

  //! Calculate the absolute value
  static inline Float16 Absolute(const Float16 &x) {
    Float16 abs(x.value_);
    uint16_t *p = reinterpret_cast<uint16_t *>(&abs.value_);
    *p &= 0x7fff;
    return abs;
  }

 private:
  __fp16 value_;
};
#endif

// Check size of Float16
static_assert(sizeof(Float16) == 2, "Float16 must be aligned with 2 bytes");

}  // namespace ailego
}  // namespace zvec
