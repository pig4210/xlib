/**
  \file  xswap.h
  \brief 定义了 swap 的相关模板。

  \version    3.0.0.260430

  \author     triones
  \date       2014-01-07

  \details    bswap 在开启完全优化情况下，会被编译成只有一条 bswap 的汇编指令。
              不使用 std::byteswap 是为了额外支持 enum 类型，同时兼容 C++17 。如此，目前损失 constexpr 性能。

  \section history 版本记录

  - 2014-01-07 从 mkmem 移植入 xlib ，虽然 C++0x 可以实现模版定义与实现分离，但不适用于 LIB ，为记。 1.0 。
  - 2016-11-14 适配 Linux g++ 。 1.1 。
  - 2019-09-20 重构 bswap 。 2.0 。
  - 2019-11-05 升级声明。 2.1 。
  - 2021-08-05 升级定义。 2.2 。
  - 2026-04-30 替换宏，升级定义。 3.0 。
*/
#ifndef _xlib_xswap_H_
#define _xlib_xswap_H_

#include <cstdint>
#include <algorithm>
#include <type_traits>

#ifdef _WIN32
#include <intrin.h>
#endif

namespace xlib {

#ifdef _WIN32
inline std::uint16_t xbswap16(std::uint16_t v) noexcept { return _byteswap_ushort(v); }
inline std::uint32_t xbswap32(std::uint32_t v) noexcept { return _byteswap_ulong(v); }
inline std::uint64_t xbswap64(std::uint64_t v) noexcept { return _byteswap_uint64(v); }
#else
inline std::uint16_t xbswap16(std::uint16_t v) noexcept { return __builtin_bswap16(v); }
inline std::uint32_t xbswap32(std::uint32_t v) noexcept { return __builtin_bswap32(v); }
inline std::uint64_t xbswap64(std::uint64_t v) noexcept { return __builtin_bswap64(v); }
#endif

/**
  用于翻转数值。
  \param    value   数值。
  \return           翻转后的原类型数值。

  \code
    bswap(0x12345678) == 0x78563412;
    bswap((short)0x1234) == 0x3412;
  \endcode

  \note
    注意到：无法实现 constexpr 。
    有符号类型先转成无符号类型再翻转。
*/
template <typename T> inline constexpr
std::enable_if_t<(std::is_integral_v<T> || std::is_enum_v<T>) && sizeof(T) == sizeof(std::uint8_t), T>
bswap(const T& value) noexcept {
  return value;
}

template <typename T> inline constexpr
std::enable_if_t<(std::is_integral_v<T> || std::is_enum_v<T>) && sizeof(T) == sizeof(std::uint16_t), T>
bswap(const T& value) noexcept {
  return static_cast<T>(xbswap16(static_cast<std::uint16_t>(value)));
}

template <typename T> inline constexpr
std::enable_if_t<(std::is_integral_v<T> || std::is_enum_v<T>) && sizeof(T) == sizeof(std::uint32_t), T>
bswap(const T& value) noexcept {
  return static_cast<T>(xbswap32(static_cast<std::uint32_t>(value)));
}

template <typename T> inline constexpr
std::enable_if_t<(std::is_integral_v<T> || std::is_enum_v<T>) && sizeof(T) == sizeof(std::uint64_t), T>
bswap(const T& value) noexcept {
  return static_cast<T>(xbswap64(static_cast<std::uint64_t>(value)));
}

/**
  当 A > B 时，对调两值，并返回真。否则不变，返回假。
  \param    a   任意类型非常量值。
  \param    b   任意类型非常量值。
  \return       返回是否对调两值。

  \code
    void* a = 0x5;
    void* b = 0x1;
    seqswap(a, b); // 返回 true ，并且 a == 1，b == 5 。
  \endcode

  \note
    依赖 operator< ，因此适用于任意实现了 operator< 的类型。
*/
template <typename T> inline
bool seqswap(T& a, T& b) {
  // 不用 >= ，避免某些类型只实现了 < 运算符。
  if (!(b < a)) return false;
  std::swap(a, b);
  return true;
}

}  // namespace xlib

#endif  // _xlib_xswap_H_
