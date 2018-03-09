/*!
  \file  crc.h
  \brief crc.h定义了crc16算法、crc32算法、crc64、crc ccitt算法

  \version    5.0.1707.2418
  \note       For All

  \author     triones
  \date       2013-03-19
*/
#ifndef _XLIB_CRC_H_
#define _XLIB_CRC_H_

#include <string>

#include "xlib_base.h"

/*!
  生成指定数据的crc16
  \param    data    指定需要计算crc16的数据
  \return           返回crc16值

  \code
    std::cout << crc16(std::string("012345"));
  \endcode
*/
uint16 crc16(const void* data, const size_t size);

/*!
  生成指定数据的crc32
  \param    data    指定需要计算crc32的数据
  \return           返回crc32值

  \code
    std::cout << crc32(std::string("012345"));
  \endcode
*/
uint32 crc32(const void* data, const size_t size);

/*!
  生成指定数据的crc64
  \param    data    指定需要计算crc64的数据
  \return           返回crc64值

  \code
    std::cout << crc64(std::string("012345"));
  \endcode
*/
uint64 crc64(const void* data, const size_t size);

/*!
  生成指定数据的crc ccitt
  \param    data    指定需要计算crc ccitt的数据
  \return           返回crc ccitt值

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