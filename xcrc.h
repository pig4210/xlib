/**
  \file  xcrc.h
  \brief 定义了 CRC 算法模板。支持 crc16 、 crc32 、 crc64 、crcccitt 。

  \version    3.0.0.190929

  \author     triones
  \date       2013-03-19
  
  \section history 版本记录
  
  - 2013-03-19 新建 crc32 函数。 0.1 。
  - 2013-03-20 修正溢出 Bug 及优化。 0.2 。
  - 2013-11-30 新增 crc64 函数。 0.3 。
  - 2014-02-18 新增 crc16 函数。 0.4 。
  - 2016-12-16 适配 Linux g++ 。新增 crcccitt 函数。 1.0 。
  - 2017-07-24 改进函数定义。 1.1 。
  - 2019-09-19 修改为模板，使用了简单的单例。 2.0 。
  - 2019-09-29 引入新特性重构，解决非线程安全的问题。 3.0 。
*/
#ifndef _XLIB_XCRC_H_
#define _XLIB_XCRC_H_

#include <climits>
#include <string>

// C++14 多行 constexpr 。
template<typename T, T N> constexpr T GetCrcTableValue(const T i)
  {
  T crc = i;
  for(size_t j = 0; j < CHAR_BIT; ++j)
    {
    crc = (crc >> 1) ^ ((crc & 1) ? N : 0);
    }
  return crc;
  }

// 除了 define ，别无他法。
#define XCV GetCrcTableValue<T, N>

// C++14 模板变量。编译期初始化、全局唯一、不使用不编译。
template<typename T, T N> const T CrcTable[0x100]{
  XCV(0x00), XCV(0x01), XCV(0x02), XCV(0x03), XCV(0x04), XCV(0x05), XCV(0x06), XCV(0x07), XCV(0x08), XCV(0x09), XCV(0x0A), XCV(0x0B), XCV(0x0C), XCV(0x0D), XCV(0x0E), XCV(0x0F),
  XCV(0x10), XCV(0x11), XCV(0x12), XCV(0x13), XCV(0x14), XCV(0x15), XCV(0x16), XCV(0x17), XCV(0x18), XCV(0x19), XCV(0x1A), XCV(0x1B), XCV(0x1C), XCV(0x1D), XCV(0x1E), XCV(0x1F),
  XCV(0x20), XCV(0x21), XCV(0x22), XCV(0x23), XCV(0x24), XCV(0x25), XCV(0x26), XCV(0x27), XCV(0x28), XCV(0x29), XCV(0x2A), XCV(0x2B), XCV(0x2C), XCV(0x2D), XCV(0x2E), XCV(0x2F),
  XCV(0x30), XCV(0x31), XCV(0x32), XCV(0x33), XCV(0x34), XCV(0x35), XCV(0x36), XCV(0x37), XCV(0x38), XCV(0x39), XCV(0x3A), XCV(0x3B), XCV(0x3C), XCV(0x3D), XCV(0x3E), XCV(0x3F),
  XCV(0x40), XCV(0x41), XCV(0x42), XCV(0x43), XCV(0x44), XCV(0x45), XCV(0x46), XCV(0x47), XCV(0x48), XCV(0x49), XCV(0x4A), XCV(0x4B), XCV(0x4C), XCV(0x4D), XCV(0x4E), XCV(0x4F),
  XCV(0x50), XCV(0x51), XCV(0x52), XCV(0x53), XCV(0x54), XCV(0x55), XCV(0x56), XCV(0x57), XCV(0x58), XCV(0x59), XCV(0x5A), XCV(0x5B), XCV(0x5C), XCV(0x5D), XCV(0x5E), XCV(0x5F),
  XCV(0x60), XCV(0x61), XCV(0x62), XCV(0x63), XCV(0x64), XCV(0x65), XCV(0x66), XCV(0x67), XCV(0x68), XCV(0x69), XCV(0x6A), XCV(0x6B), XCV(0x6C), XCV(0x6D), XCV(0x6E), XCV(0x6F),
  XCV(0x70), XCV(0x71), XCV(0x72), XCV(0x73), XCV(0x74), XCV(0x75), XCV(0x76), XCV(0x77), XCV(0x78), XCV(0x79), XCV(0x7A), XCV(0x7B), XCV(0x7C), XCV(0x7D), XCV(0x7E), XCV(0x7F),
  XCV(0x80), XCV(0x81), XCV(0x82), XCV(0x83), XCV(0x84), XCV(0x85), XCV(0x86), XCV(0x87), XCV(0x88), XCV(0x89), XCV(0x8A), XCV(0x8B), XCV(0x8C), XCV(0x8D), XCV(0x8E), XCV(0x8F),
  XCV(0x90), XCV(0x91), XCV(0x92), XCV(0x93), XCV(0x94), XCV(0x95), XCV(0x96), XCV(0x97), XCV(0x98), XCV(0x99), XCV(0x9A), XCV(0x9B), XCV(0x9C), XCV(0x9D), XCV(0x9E), XCV(0x9F),
  XCV(0xA0), XCV(0xA1), XCV(0xA2), XCV(0xA3), XCV(0xA4), XCV(0xA5), XCV(0xA6), XCV(0xA7), XCV(0xA8), XCV(0xA9), XCV(0xAA), XCV(0xAB), XCV(0xAC), XCV(0xAD), XCV(0xAE), XCV(0xAF),
  XCV(0xB0), XCV(0xB1), XCV(0xB2), XCV(0xB3), XCV(0xB4), XCV(0xB5), XCV(0xB6), XCV(0xB7), XCV(0xB8), XCV(0xB9), XCV(0xBA), XCV(0xBB), XCV(0xBC), XCV(0xBD), XCV(0xBE), XCV(0xBF),
  XCV(0xC0), XCV(0xC1), XCV(0xC2), XCV(0xC3), XCV(0xC4), XCV(0xC5), XCV(0xC6), XCV(0xC7), XCV(0xC8), XCV(0xC9), XCV(0xCA), XCV(0xCB), XCV(0xCC), XCV(0xCD), XCV(0xCE), XCV(0xCF),
  XCV(0xD0), XCV(0xD1), XCV(0xD2), XCV(0xD3), XCV(0xD4), XCV(0xD5), XCV(0xD6), XCV(0xD7), XCV(0xD8), XCV(0xD9), XCV(0xDA), XCV(0xDB), XCV(0xDC), XCV(0xDD), XCV(0xDE), XCV(0xDF),
  XCV(0xE0), XCV(0xE1), XCV(0xE2), XCV(0xE3), XCV(0xE4), XCV(0xE5), XCV(0xE6), XCV(0xE7), XCV(0xE8), XCV(0xE9), XCV(0xEA), XCV(0xEB), XCV(0xEC), XCV(0xED), XCV(0xEE), XCV(0xEF),
  XCV(0xF0), XCV(0xF1), XCV(0xF2), XCV(0xF3), XCV(0xF4), XCV(0xF5), XCV(0xF6), XCV(0xF7), XCV(0xF8), XCV(0xF9), XCV(0xFA), XCV(0xFB), XCV(0xFC), XCV(0xFD), XCV(0xFE), XCV(0xFF)
  };

#undef XCV

/// CRC 基本模板。
template<typename T, T N, T V, bool R, typename P>
T XCRC(const P* const data, const size_t size)
  {
  T ret = V;
  const size_t len = (nullptr == data) ? 0 : (size * sizeof(P));
  const uint8_t* const buf = (const uint8_t*)data;
  for(size_t i = 0; i < len; ++i)
    {
    ret = CrcTable<T, N>[(ret & 0xFF) ^ buf[i]] ^ (ret >> 8);
    }
  return R ? ~ret : ret;
  }
template<typename T, T N, T V, bool R>
T XCRC(const void* const data, const size_t size)
  {
  return XCRC<T, N, V, R>((const char*)data, size);
  }
template<typename T, T N, T V, bool R, typename S>
T XCRC(const std::basic_string<S>& s)
  {
  return XCRC<T, N, V, R>(s.c_str(), s.size());
  }

//////////////////////////////////////////////////////////////////////////
// 因需要使用重载，除了 define ，别无他法。
/**
  生成指定数据的 crc16 。
  \param    data    指定需要计算 crc16 的数据。
  \param    size    指定需要计算 crc16 的数据长度（以相应类型字计）。
  \return           返回 crc16 值。

  \code
    std::cout << crc16(std::string("012345"));
  \endcode
*/
#define crc16           (XCRC<uint16_t, 0xA001, 0, false>)
/**
  生成指定数据的 crc32 。
  \param    data    指定需要计算 crc32 的数据。
  \param    size    指定需要计算 crc32 的数据长度（以相应类型字计）。
  \return           返回 crc32 值。

  \code
    std::cout << crc32(std::string("012345"));
  \endcode
*/
#define crc32           (XCRC<uint32_t, 0xEDB88320, 0xFFFFFFFF, true>)
/**
  生成指定数据的 crc64 。
  \param    data    指定需要计算 crc64 的数据。
  \param    size    指定需要计算 crc64 的数据长度（以相应类型字计）。
  \return           返回 crc64 值。

  \code
    std::cout << crc64(std::string("012345"));
  \endcode
*/
#define crc64           (XCRC<uint64_t, 0xC96C5795D7870F42, 0xFFFFFFFFFFFFFFFF, true>)
/**
  生成指定数据的 crcccitt 。
  \param    data    指定需要计算 crcccitt 的数据。
  \param    size    指定需要计算 crcccitt 的数据长度（以相应类型字计）。
  \return           返回 crcccitt 值。

  \code
    std::cout << crcccitt(std::string("012345"));
  \endcode
*/
#define crcccitt        (XCRC<uint16_t, 0x8408, 0xFFFF, false>)

#endif  // _XLIB_XCRC_H_