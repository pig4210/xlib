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
  - 2020-03-06 引入可变参数模板，重构。 4.0 。
*/
#ifndef _XLIB_XCRC_H_
#define _XLIB_XCRC_H_

#include <climits>
#include <string>
#include <array>

template<typename T, T N> constexpr T GetCrcTableValue(const T i)
  {
  T crc = i;
  for(size_t j = 0; j < CHAR_BIT; ++j)
    {
    crc = (crc >> 1) ^ ((crc & 1) ? N : 0);
    }
  return crc;
  }

template<typename T, T N, std::size_t... I>
constexpr auto __forceinline GetCrcTable(std::index_sequence<I...>)
  {
  return std::array<T, sizeof...(I)>{GetCrcTableValue<T, N>(I)...};
  }

/// CRC 基本模板。
template<typename T, T N, T V, bool R, typename P>
T XCRC(const P* const data, const size_t size)
  {
  constexpr auto CrcTable = GetCrcTable<T, N>(std::make_index_sequence<0x100>{});
  T ret = V;
  const size_t len = (nullptr == data) ? 0 : (size * sizeof(P));
  const uint8_t* const buf = (const uint8_t*)data;
  for(size_t i = 0; i < len; ++i)
    {
    ret = CrcTable[(ret & 0xFF) ^ buf[i]] ^ (ret >> 8);
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