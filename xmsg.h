/**
  \file  xmsg.h
  \brief 定义了信息组织的基本类，类似标准库的 ostreamstring 。

  \version    3.0.0.191103
  \note       For All

  \author     triones
  \date       2011-07-22

  \section history 版本记录

  - 2012-06-06 从 xlog 中分离，重新定义为 xmsg 。 0.1 。
  - 2012-06-06 考虑 xmsg 继承 mem_buffer ，需要新建不少函数，暂不施行。
  - 2012-07-19 优化 prt 。 0.1.1
  - 2012-10-09 新增对 int64 的支持。其它格式化作了些优化。 0.2 。
  - 2012-10-23 使 xmsg 信息组织后数据包含结尾 0 。 xmsg 初始时为 "\0" 而非空串。
  - 2012-10-23 重载 end 、 empty 。 0.3 。
  - 2013-03-05 重载 clear 以修复缓冲清空后的小 BUG 。 0.3.1 。
  - 2014-01-11 引入 SGISTL ，修改适应标准库。 1.0 。
  - 2014-04-09 修改一些数据的输出格式。 1.1 。
  - 2016-07-20 添加 xmsg 构造。 1.2 。
  - 2016-11-15 适配 Linux g++ 。处理不再附加结尾 0 。 1.3 。
  - 2019-09-25 重构 xmsg 。 2.0 。
  - 2019-11-03 引入 char8_t 、 u8string 。 3.0 。
*/
#ifndef _XLIB_XMSG_H_
#define _XLIB_XMSG_H_

#include <string>
#include <cstdio>
#include <cstdarg>
#include <climits>

#include "as_ws_u8.h"

class xmsg : public std::string
  {
  public:
    xmsg() {}
    xmsg(const std::string& as):std::string(as) {}
    xmsg(const std::wstring& ws):std::string(ws2as(ws)) {}
    xmsg(const std::u8string& u8):std::string(u82as(u8)) {}
  public:
    /// 指定格式输出。
    xmsg& prt(const char* const fmt, ...)
      {
      if(nullptr == fmt) return *this;
      va_list ap;
      va_start(ap, fmt);
      const auto need = std::vsnprintf(nullptr, 0, fmt, ap);
      va_end(ap);
      // 格式化失败，不做处理。
      if(need <= 0)  return *this;
      reserve(size() + need + 1);
      char* lpend = data() + size();
      // 注意到，g++ 中 ap 好像会被修改，所以需要重做。
      va_start(ap, fmt);
      std::vsnprintf(lpend, capacity() - size(), fmt, ap);
      va_end(ap);
      append(lpend, need);
      return *this;
      }
    /// 输出 dec 值。
    xmsg& operator<<(const int8_t& v)
      {
      return prt("%hhi", v);
      }
    /// 输出 hex(XX)。
    xmsg& operator<<(const uint8_t& v)
      {
      return prt("%02X", v);
      }
    /// 输出 dec 值。
    xmsg& operator<<(const int16_t& v)
      {
      return prt("%hi", v);
      }
    /// 输出 hex(XXXX)。
    xmsg& operator<<(const uint16_t& v)
      {
      return prt("%04X", v);
      }
    /// 输出 dec 值。
    xmsg& operator<<(const int32_t& v)
      {
      return prt("%i", v);
      }
    /// 输出 hex(XXXXXXXX)。
    xmsg& operator<<(const uint32_t& v)
      {
      return prt("%08X", v);
      }
    /// 输出 dec 值。
    xmsg& operator<<(const int64_t& v)
      {
      return prt("%lli", v);
      }
    /// 输出 hex(XXXXXXXXXXXXXXXX)。
    xmsg& operator<<(const uint64_t& v)
      {
      return prt("%08X%08X", (uint32_t)(v >> (CHAR_BIT * sizeof(uint32_t))), (uint32_t)v);
      }
    /// 输出 hex 指针。
    xmsg& operator<<(const void* const v)
      {
      return operator<<((size_t)v);
      }
    /// 输出 :true :false。
    xmsg& operator<<(const bool& v)
      {
      return operator<<(v ? ":true" : ":false");
      }
    /// 输出 字符。
    xmsg& operator<<(const char& v)
      {
      append(1, v);
      return *this;
      }
    /// 输出 字符串。
    xmsg& operator<<(const char* const v)
      {
      if(nullptr != v) append(v);
      return *this;
      }
    /// 输出 字符串。
    xmsg& operator<<(const std::string& v)
      {
      append(v);
      return *this;
      }
    /// 输出 UNICCODE 转化 ANSI 字符。
    xmsg& operator<<(const wchar_t& v)
      {
      append(ws2as(std::wstring(1, v)));
      return *this;
      }
    /// 输出 UNICCODE 字符串转化 ANSI 字符串。
    xmsg& operator<<(const wchar_t* const v)
      {
      if(nullptr != v) append(ws2as(v));
      return *this;
      }
    /// 输出 UNICCODE 字符串转化 ANSI 字符串。
    xmsg& operator<<(const std::wstring& v)
      {
      append(ws2as(v));
      return *this;
      }
    /// 输出 UTF-8 转换 ANSI 字符。
    xmsg& operator<<(const char8_t& v)
      {
      append(u82as(std::u8string(1, v)));
      return *this;
      }
    /// 输出 UTF-8 字符串转换 ANSI 字符串。
    xmsg& operator<<(const char8_t* v)
      {
      if(nullptr != v) append(u82as(v));
      return *this;
      }
    /// 输出 UTF-8 字符串转化 ANSI 字符串。
    xmsg& operator<<(const std::u8string& v)
      {
      append(u82as(v));
      return *this;
      }
    /// 输出 dec 浮点数。
    xmsg& operator<<(const float& v)
      {
      return prt("%f", v);
      }
    /// 输出 dec 浮点数。
    xmsg& operator<<(const double& v)
      {
      return prt("%f", v);
      }
    /// 输出 内容。
    xmsg& operator<<(const xmsg& v)
      {
      append(v);
      return *this;
      }
  };

#endif  // _XLIB_XMSG_H_