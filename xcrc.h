/**
  \file  xcrc.h
  \brief 定义了 CRC 算法模板。支持 crc16 、 crc32 、 crc64 、crcccitt 。

  \version    2.0.0.190919
  \note       For All

  \author     triones
  \date       2013-03-19
  
  \section more 额外说明

  - CRC Table 使用 static ，初始化时比较麻烦。且开放给外部时，不容易做成 const 。
  - CRC Table 还需要开放大小查询。

  \section history 版本记录
  
  - 2013-03-19 新建 crc32 函数。 0.1 。
  - 2013-03-20 修正溢出 Bug 及优化。 0.2 。
  - 2013-11-30 新增 crc64 函数。 0.3 。
  - 2014-02-18 新增 crc16 函数。 0.4 。
  - 2016-12-16 适配 Linux g++ 。新增 crcccitt 函数。 1.0 。
  - 2017-07-24 改进函数定义。 1.1 。
  - 2019-09-19 修改为模板，使用了简单的单例。 2.0 。
*/
#ifndef _XLIB_XCRC_H_
#define _XLIB_XCRC_H_

#include <climits>
#include <string>

/// CRC 基本模板。使用简单的单例，令 CRC Table 自动且只初始化一次。（注意：刻意忽略非线程安全。）
template<typename T, T N, T V, bool R> class XCRC
  {
  private:
    /// 初始化 CRC Table 。
    constexpr XCRC()
      {
      for(T i = 0; i < std::size(CrcTable); ++i)
        {
        T crc = i;
        for(size_t j = 0; j < 8; ++j)
          {
          crc = (crc >> 1) ^ ((crc & 1) ? N : 0);
          }
        CrcTable[i] = crc;
        }
      }
    XCRC(const XCRC&) = delete;
    XCRC& operator=(const XCRC&) = delete;
  public:
    /// 单例仅返回常量对象。
    static const XCRC& instance()
      {
      static const XCRC obj;
      return obj;
      }
    T operator()(const void* data, size_t size) const
      {
      T ret = V;
      size = (nullptr == data) ? 0 : size;
      const uint8_t* const buf = (const uint8_t*)data;
      for(size_t i = 0; i < size; ++i)
        {
        ret = CrcTable[(ret & 0xFF) ^ buf[i]] ^ (ret >> 8);
        }
      return R ? ~ret : ret;
      }
    template<typename S> T operator()(const std::basic_string<S>& s) const
      {
      return this->operator()(s.c_str(), s.size() * sizeof(S));
      }
  public:
    /// Crc Table 开放，允许被查询（但不允许修改），大小可查询。
    T CrcTable[0x100];
  };

//////////////////////////////////////////////////////////////////////////
/**
  生成指定数据的 crc16 。
  \param    data    指定需要计算 crc16 的数据。
  \return           返回 crc16 值。

  \code
    std::cout << crc16(std::string("012345"));
  \endcode
*/
#define crc16           (XCRC<uint16_t, 0xA001, 0, false>::instance())
/**
  生成指定数据的 crc32 。
  \param    data    指定需要计算 crc32 的数据。
  \return           返回 crc32 值。

  \code
    std::cout << crc32(std::string("012345"));
  \endcode
*/
#define crc32           (XCRC<uint32_t, 0xEDB88320, 0xFFFFFFFF, true>::instance())
/**
  生成指定数据的 crc64 。
  \param    data    指定需要计算 crc64 的数据。
  \return           返回 crc64 值。

  \code
    std::cout << crc64(std::string("012345"));
  \endcode
*/
#define crc64           (XCRC<uint64_t, 0xC96C5795D7870F42, 0xFFFFFFFFFFFFFFFF, true>::instance())
/**
  生成指定数据的 crcccitt 。
  \param    data    指定需要计算 crcccitt 的数据。
  \return           返回 crcccitt 值。

  \code
    std::cout << crcccitt(std::string("012345"));
  \endcode
*/
#define crcccitt        (XCRC<uint16_t, 0x8408, 0xFFFF, false>::instance())

#endif  // _XLIB_XCRC_H_