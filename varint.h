﻿/**
  \file  varint.h
  \brief 定义了 zig 、 zag 、 varint 相关操作。

  \version    2.0.0.200313
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
#ifndef _XLIB_VARINT_H_
#define _XLIB_VARINT_H_

#include <climits>
#include <string>
#include <array>

template<typename T> constexpr
std::enable_if_t<std::is_signed<T>::value || std::is_unsigned<T>::value,
typename std::make_unsigned<T>::type> inline zig(const T& value)
  {
  using U = typename std::make_unsigned<T>::type;
  // 无符号值或枚举值，不转换。
  if constexpr (std::is_unsigned<T>::value)
    {
    return (U)value;
    }
  // 有符号值，转换成无符号值。
  else
    {
    const T v = value;
    return (U)((v << 1) ^ (v >> (sizeof(T) * CHAR_BIT - 1)));
    }
  }

template<typename T> constexpr
std::enable_if_t<std::is_signed<T>::value || std::is_unsigned<T>::value,
typename std::make_signed<T>::type> inline zag(const T& value)
  {
  using S = typename std::make_signed<T>::type;
  // 有符号值，不转换。
  if constexpr (std::is_signed<T>::value)
    {
    return (S)value;
    }
  // 无符号值，转换成有符号值。
  else
    {
    const S v = (S)value;
    return ((-(v & 0x01)) ^ ((v >> 1) & ~((T)1 << (sizeof(T) * CHAR_BIT - 1))));
    }
  }

template<typename T, std::enable_if_t<std::is_signed<T>::value || std::is_unsigned<T>::value, int> = 0>
class varint : public std::array<uint8_t, sizeof(T) / CHAR_BIT + 1 + sizeof(T)>
  {
  private:
    T _value;
  public:
    constexpr varint(const T& value):_value(value)
      {
      auto v = zig(value);
      for(auto& pv : *this)
        {
        const auto vv = (uint8_t)(v & 0x7F);
        v >>= (CHAR_BIT - 1);
        if(0 == v)
          {
          pv = vv;
          break;
          }
        pv = vv | 0x80;
        }
      }
    constexpr size_t size() const noexcept
      {
      size_t n = 0;
      for(const auto& v : *this)
        {
        ++n;
        // g++ 这里有 may be used uninitialized in this function 警告，可忽略。
        if(0 == (v & 0x80)) break;
        }
      return n;
      }
    constexpr operator T() const noexcept
      {
      return _value;
      }
    constexpr T operator()() const noexcept
      {
      return _value;
      }
    constexpr varint(const char* p):_value(0)
      {
      using U = typename std::make_unsigned<T>::type;
      U v = 0;
      size_t count = 0;
      for(auto& pv : *this)
        {
        pv = *p; ++p;
        v |= ((pv & 0x7F) << (count * (CHAR_BIT - 1)));
        ++count;
        if(0 == (pv & 0x80))
          {
          _value = (std::is_signed<T>::value) ? (T)zag((U)v) : (T)v;
          break;
          }
        }
      }
    template<typename Ty,
    std::enable_if_t<(sizeof(Ty) == 1) || std::is_void<Ty>::value, int> = 0>
    varint(const Ty* p):varint((const char*)p)
      {
      }
  };

#endif  // _XLIB_VARINT_H_