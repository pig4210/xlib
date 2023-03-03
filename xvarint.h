/**
  \file  xvarint.h
  \brief 定义了 zig 、 zag 、 varint 相关操作。

  \version    2.0.0.230227
  \note       For All

  \author     triones
  \date       2017-09-01

  \section history 版本记录

  - 2017-09-05 添加 varint 实现。 1.0 。
  - 2017-09-11 修正处理有符号时的错误。 1.0.1 。
  - 2019-10-21 改进。1.1 。
  - 2019-11-06 重构 zig 、 zag 。 1.2 。
  - 2020-03-13 重构 varint 。 2.0 。
*/
#ifndef _XLIB_XVARINT_H_
#define _XLIB_XVARINT_H_

#include <array>
#include <climits>
#include <string>

namespace xlib {

/// zig 用于处理有符号数为无符号数，但返回还是原类型。
template <typename T> inline constexpr
std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>, T>
xzig(const T& v) {
  using U = typename std::make_unsigned_t<T>;
  if constexpr (std::is_signed_v<T>) {
    // 有符号值，转换成无符号值。
    return ((v << 1) ^ (v >> (sizeof(T) * CHAR_BIT - 1)));
  } else {
    // 无符号值或枚举值，不转换。
    return v;
  }
}

/// zag 根据输入类型，处理有符号数，返回还是原类型。
template <typename T> inline constexpr
std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>, T>
xzag(const T& v) {
  using S = typename std::make_signed_t<T>;
  if constexpr (std::is_signed_v<T>) {
    // 有符号值，转换成有符号值。
    return ((-(v & 0x01)) ^ ((v >> 1) & ~((T)1 << (sizeof(T) * CHAR_BIT - 1))));
  } else {
    // 无符号值，不转换。
    return v;
  }
}

template <typename T, std::enable_if_t<std::is_integral_v<T> || std::is_enum_v<T>, int> = 0>
class xvarint : public std::array<uint8_t, sizeof(T) / CHAR_BIT + 1 + sizeof(T)> {
 public:
  using base = std::array<uint8_t, sizeof(T) / CHAR_BIT + 1 + sizeof(T)>;

 private:
  T _value;

 public:
  constexpr xvarint(const T& value)
      : std::array<uint8_t, sizeof(T) / CHAR_BIT + 1 + sizeof(T)>(),
        _value(value) {
    // g++ 这里有 will be initialized after 警告，可忽略。
    using U = typename std::make_unsigned_t<T>;
    auto v = (U)xzig(value);
    for (auto& pv : *this) {
      const auto vv = (uint8_t)(v & 0x7F);
      v >>= (CHAR_BIT - 1);
      if (0 == v) {
        pv = vv;
        break;
      }
      pv = vv | 0x80;
    }
  }
  constexpr uint8_t* data() const noexcept {
    return (uint8_t*)base::data();
  }
  constexpr size_t size() const noexcept {
    size_t n = 0;
    for (const auto& v : *this) {
      ++n;
      // g++ 这里有 may be used uninitialized in this function 警告，可忽略。
      if (0 == (v & 0x80)) break;
    }
    return n;
  }
  constexpr operator T() const noexcept {
    return _value;
  }
  constexpr T operator()() const noexcept {
    return _value;
  }
  constexpr xvarint(const char* p) : _value(T()) {
    using U = typename std::make_unsigned_t<T>;
    U v = 0;
    size_t count = 0;
    for (auto& pv : *this) {
      pv = *p;
      ++p;
      v |= ((pv & 0x7F) << (count * (CHAR_BIT - 1)));
      ++count;
      if (0 == (pv & 0x80)) {
        _value = xzag((T)v);
        break;
      }
    }
  }
  template <typename Ty, std::enable_if_t<(sizeof(Ty) == 1) || std::is_void<Ty>::value, int> = 0>
  xvarint(const Ty* p) : xvarint((const char*)p) {}
};

}  // namespace xlib

#endif  // _XLIB_XVARINT_H_