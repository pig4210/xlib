/**
  \file  xswap.h
  \brief 定义了 swap 的相关模板。

  \version    2.1.0.191105

  \author     triones
  \date       2014-01-07

  \details    bswap 在开启完全优化情况下，会被编译成只有一条 bswap 的汇编指令。
  
  \section history 版本记录

  - 2014-01-07 从 mkmem 移植入 xlib ，虽然 C++0x 可以实现模版定义与实现分离，但不适用于 LIB ，为记。 1.0 。
  - 2016-11-14 适配 Linux g++ 。 1.1 。
  - 2019-09-20 重构 bswap 。 2.0 。
  - 2019-11-05 升级声明。 2.1 。
*/
#ifndef _XLIB_XSWAP_H_
#define _XLIB_XSWAP_H_

#include <cstdint>
#ifdef _WIN32
#define bswap16   _byteswap_ushort
#define bswap32   _byteswap_ulong
#define bswap64   _byteswap_uint64
#else
#define bswap16   __builtin_bswap16
#define bswap32   __builtin_bswap32
#define bswap64   __builtin_bswap64
#endif
#include <algorithm>

/// bswap 模板，是 bswap 函数的实现细节。
template<size_t N> inline void bswap_type(uint8_t* mem)
  {
  /*
    模板内部调用了特化模板。  
    注意：三个基本大小必须特化，否则无法优化编译成一个 bswap 指令。  
    常量会直接在编译阶段直接优化成值。
  */
  size_t len = N;
  while(len != 0)
    {
    switch(len)
      {
      case 1:bswap_type<1>(mem); return;
      case 2:
      case 3:bswap_type<2>(mem); return;
      case 4:
      case 5:bswap_type<4>(mem); return;
      case 6:
      case 7:bswap_type<4>(mem); bswap_type<2>(mem + 4); return;
      default:
        bswap_type<8>(mem);
        len -= 8;
        mem += 8;
      }
    }
  }
template<> inline void bswap_type<1>(uint8_t*)
  {
  }
template<> inline void bswap_type<2>(uint8_t* mem)
  {
  *(uint16_t*)mem = bswap16(*(const uint16_t*)mem);
  }
template<> inline void bswap_type<4>(uint8_t* mem)
  {
  *(uint32_t*)mem = bswap32(*(const uint32_t*)mem);
  }
template<> inline void bswap_type<8>(uint8_t* mem)
  {
  *(uint64_t*)mem = bswap64(*(const uint64_t*)mem);
  }

/**
  用于翻转数值。
  \param    values  数值。
  \return           翻转后的原类型数值。

  \code
    bswap(0x12345678) == 0x78563412;
    bswap((short)0x1234) == 0x3412;
  \endcode
*/
template<typename T>
inline std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value, T>
  bswap(const T& values)
  {
  T v = values;
  bswap_type<sizeof(T)>((uint8_t*)&v);
  return v;
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
*/
template<typename T> inline bool seqswap(T& a, T& b)
  {
  if(std::max(a, b) == b) return false;
  std::swap(a, b);
  return true;
  }

#endif  // _XLIB_XSWAP_H_
