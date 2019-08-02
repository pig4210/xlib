/**
  \file  crc.h
  \brief 定义了 crc16 、 crc32 、 crc64 、crcccitt 算法。

  \version    1.1.0.170724
  \note       For All

  \author     triones
  \date       2013-03-19

  \section history 版本记录
  
  - 2013-03-19 新建 crc32 函数。 0.1 。
  - 2013-03-20 修正溢出 Bug 及优化。 0.2 。
  - 2013-11-30 新增 crc64 函数。 0.3 。
  - 2014-02-18 新增 crc16 函数。 0.4 。
  - 2016-12-16 适配 Linux g++ 。新增 crcccitt 函数。 1.0 。
  - 2017-07-24 改进函数定义。 1.1 。
*/
#ifndef _XLIB_CRC_H_
#define _XLIB_CRC_H_

#include <string>

#include "xlib_base.h"

/**
  生成指定数据的 crc16 。
  \param    data    指定需要计算 crc16 的数据。
  \return           返回 crc16 值。

  \code
    std::cout << crc16(std::string("012345"));
  \endcode
*/
uint16 crc16(const void* data, const size_t size);

/**
  生成指定数据的 crc32 。
  \param    data    指定需要计算 crc32 的数据。
  \return           返回 crc32 值。

  \code
    std::cout << crc32(std::string("012345"));
  \endcode
*/
uint32 crc32(const void* data, const size_t size);

/**
  生成指定数据的 crc64 。
  \param    data    指定需要计算 crc64 的数据。
  \return           返回 crc64 值。

  \code
    std::cout << crc64(std::string("012345"));
  \endcode
*/
uint64 crc64(const void* data, const size_t size);

/**
  生成指定数据的 crcccitt 。
  \param    data    指定需要计算 crcccitt 的数据。
  \return           返回 crcccitt 值。

  \code
    std::cout << crcccitt(std::string("012345"));
  \endcode
*/
uint16 crcccitt(const void* data, const size_t size);

//////////////////////////////////////////////////////////////////////////
template<typename T> uint16 crc16(const std::basic_string<T>& s)
  {
  return crc16(s.c_str(), s.size() * sizeof(T));
  }

template<typename T> uint32 crc32(const std::basic_string<T>& s)
  {
  return crc32(s.c_str(), s.size() * sizeof(T));
  }

template<typename T> uint64 crc64(const std::basic_string<T>& s)
  {
  return crc64(s.c_str(), s.size() * sizeof(T));
  }

template<typename T> uint16 crcccitt(const std::basic_string<T>& s)
  {
  return crcccitt(s.c_str(), s.size() * sizeof(T));
  }

#endif  // _XLIB_CRC_H_