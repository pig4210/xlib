/**
  \file  varint.h
  \brief 定义了 zig 、 zag 、 varint 相关操作。

  \version    1.0.1.170911
  \note       For All

  \author     triones
  \date       2017-09-01

  \section history 版本记录

  - 2017-09-05 添加 varint 实现。 1.0 。
  - 2017-09-11 修正处理有符号时的错误。 1.0.1 。
*/
#ifndef _XLIB_VARINT_H_
#define _XLIB_VARINT_H_

#include <string>

#include "xlib_base.h"

/// 无符号值或枚举值，不转换。
template<typename U>
typename std::enable_if<
  !std::is_signed<U>::value, typename std::make_unsigned<U>::type>::type
  zig(const U& value)
  {
  typedef typename std::make_unsigned<U>::type UU;
  return (UU)value;
  }

/// 有符号值，转换成无符号值。
template<typename T>
typename std::enable_if<
  std::is_signed<T>::value, typename std::make_unsigned<T>::type>::type
  zig(const T& value)
  {
  typedef typename std::make_unsigned<T>::type U;
  const T v = value;
  return (U)((v << 1) ^ (v >> (sizeof(T) * 8 - 1)));
  }

/// 有符号值，不转换。
template<typename T>
typename std::enable_if<
  std::is_signed<T>::value, typename std::make_signed<T>::type>::type
  zag(const T& value)
  {
  typedef typename std::make_signed<T>::value TT;
  return (TT)value;
  }

/// 无符号值，转换成有符号值。
template<typename U>
typename std::enable_if<
  !std::is_signed<U>::value, typename std::make_signed<U>::type>::type
  zag(const U& value)
  {
  typedef typename std::make_signed<U>::type T;
  const T v = (T)value;
  return ((-(v & 0x01)) ^ ((v >> 1) & ~((T)1 << (sizeof(T) * 8 - 1))));
  }

/// 转换值，缓冲区错误、不足，将返回 0 值，否则返回写入缓冲区的大小。
template<typename T>
size_t tovarint(const T&      value,
                void*         varint,
                const size_t  size = sizeof(T) / 8 + 1 + sizeof(T))
  {
  auto v = zig(value);
  size_t count = 0;
  uint8* data = (uint8*)varint;
  XLIB_TRY
    {
    do
      {
      if(count > size) return 0;
      data[count] = (uint8)((v & 0x7F) | 0x80);
      v >>= 7;
      ++count;
      } while(v != 0);
    data[count - 1] &= 0x7F;
    }
  XLIB_CATCH
    {
    return 0;
    }
  return count;
  }

/// 转换值，数据不足，返回 0 值。目标容量不足，截断值。否则返回读取缓冲区的大小。
template<typename T>
size_t getvarint(T&             value,
                 const void*    varint,
                 const size_t   size = sizeof(T) / 8 + 1 + sizeof(T))
  {
  typedef typename std::make_unsigned<T>::type U;
  U v = 0;
  size_t count = 0;
  const uint8* data = (const uint8*)varint;
  XLIB_TRY
    {
    while(count < size)
      {
      U ch = data[count];
      v |= ((ch & 0x7F) << (count * 7));
      ++count;
      if((ch & 0x80) == 0)
        {
        value = (std::is_signed<T>::value) ? (T)zag((U)v) : (T)v;
        return count;
        }
      }
    }
  XLIB_CATCH
    {
    }
  return 0;
  }

//////////////////////////////////////////////////////////////////////////
template<typename T>
std::string tovarint(const T& value)
  {
  uint8 varint[sizeof(T) / 8 + 1 + sizeof(T)];
  auto size = tovarint(value, varint, sizeof(varint));
  return std::string((const char*)varint, size);
  }

template<typename T, typename Ty>
size_t getvarint(T& value, const std::basic_string<Ty>& varint)
  {
  return getvarint(value, varint.c_str(), varint.size() * sizeof(T));
  }

#endif  // _XLIB_VARINT_H_