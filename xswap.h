/**
  \file  xswap.h
  \brief 定义了 swap 的相关模板。

  \version    2.2.0.230224

  \author     triones
  \date       2014-01-07

  \details    bswap 在开启完全优化情况下，会被编译成只有一条 bswap 的汇编指令。

  \section history 版本记录

  - 2014-01-07 从 mkmem 移植入 xlib ，虽然 C++0x 可以实现模版定义与实现分离，但不适用于 LIB ，为记。 1.0 。
  - 2016-11-14 适配 Linux g++ 。 1.1 。
  - 2019-09-20 重构 bswap 。 2.0 。
  - 2019-11-05 升级声明。 2.1 。
  - 2021-08-05 升级定义。 2.2 。
*/
#ifndef _XLIB_XSWAP_H_
#define _XLIB_XSWAP_H_

#include <cstdint>
#include <algorithm>

namespace xlib {

#ifdef _WIN32
#define xbswap16    _byteswap_ushort
#define xbswap32    _byteswap_ulong
#define xbswap64    _byteswap_uint64
#else
#define xbswap16    __builtin_bswap16
#define xbswap32    __builtin_bswap32
#define xbswap64    __builtin_bswap64
#endif

/**
  用于翻转数值。
  \param    values  数值。
  \return           翻转后的原类型数值。

  \code
    bswap(0x12345678) == 0x78563412;
    bswap((short)0x1234) == 0x3412;
  \endcode
*/
template <typename T>
inline std::enable_if_t<(std::is_integral_v<T> ||
                         std::is_enum_v<T>)&&sizeof(T) == sizeof(uint8_t),
                        T>
bswap(const T& values) {
  return values;
}

template <typename T>
inline std::enable_if_t<(std::is_integral_v<T> ||
                         std::is_enum_v<T>)&&sizeof(T) == sizeof(uint16_t),
                        T>
bswap(const T& values) {
  return (T)xbswap16(values);
}

template <typename T>
inline std::enable_if_t<(std::is_integral_v<T> ||
                         std::is_enum_v<T>)&&sizeof(T) == sizeof(uint32_t),
                        T>
bswap(const T& values) {
  return (T)xbswap32(values);
}

template <typename T>
inline std::enable_if_t<(std::is_integral_v<T> ||
                         std::is_enum_v<T>)&&sizeof(T) == sizeof(uint64_t),
                        T>
bswap(const T& values) {
  return (T)xbswap64(values);
}

#undef xbswap16
#undef xbswap32
#undef xbswap64

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
*/
template <typename T>
inline bool seqswap(T& a, T& b) {
  if (std::max(a, b) == b) return false;
  std::swap(a, b);
  return true;
}

}  // namespace xlib

#endif  // _XLIB_XSWAP_H_
