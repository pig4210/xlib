/**
  \file  xmsg.h
  \brief 定义了信息组织的基本类，类似标准库的 ostreamstring 。

  \version    4.0.0.200309
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
  - 2020-03-09 改变基类为 wstring 。 4.0 。
*/
#ifndef _XLIB_XMSG_H_
#define _XLIB_XMSG_H_

#include <string>
#include <cstdio>
#include <cstdarg>
#include <climits>

#include "as_ws_u8.h"

class xmsg : public std::wstring
  {
  public:
    xmsg() {}
    xmsg(const std::string& as):std::wstring(as2ws(as)) {}
    xmsg(const std::wstring& ws):std::wstring(ws) {}
    xmsg(const std::u8string& u8):std::wstring(u82ws(u8)) {}
  public:
    /// 指定格式输出。
    xmsg& prt(const wchar_t* const fmt, ...)
      {
      if(nullptr == fmt) return *this;
      va_list ap;
      for(size_t rest = 0x10; rest; rest *= 2)
        {
        // 注意到 wstring.append 无法使用 reverse 的缓冲，将导致异常，故此自建缓冲。
        std::unique_ptr<wchar_t[]> p(new wchar_t[rest]);
        // 注意到，g++ 中 ap 好像会被修改，所以需要重做。
        va_start(ap, fmt);
        // 无 vsnprintf 等价版本，vswprintf 无法检查需要多少缓冲。只能反复尝试。
        const auto need = std::vswprintf(p.get(), rest, fmt, ap);
        va_end(ap);
        if(need >= 0)
          {
          append(p.get(), need);
          break;
          }
        }
      return *this;
      }
    /// 输出 dec 值。
    xmsg& operator<<(const int8_t& v)
      {
      return prt(L"%hhi", v);
      }
    /// 输出 hex(XX)。
    xmsg& operator<<(const uint8_t& v)
      {
      return prt(L"%02X", v);
      }
    /// 输出 dec 值。
    xmsg& operator<<(const int16_t& v)
      {
      return prt(L"%hi", v);
      }
    /// 输出 hex(XXXX)。
    xmsg& operator<<(const uint16_t& v)
      {
      return prt(L"%04X", v);
      }
    /// 输出 dec 值。
    xmsg& operator<<(const int32_t& v)
      {
      return prt(L"%i", v);
      }
    /// 输出 hex(XXXXXXXX)。
    xmsg& operator<<(const uint32_t& v)
      {
      return prt(L"%08X", v);
      }
    /// 输出 dec 值。
    xmsg& operator<<(const int64_t& v)
      {
      return prt(L"%lli", v);
      }
    /// 输出 hex(XXXXXXXXXXXXXXXX)。
    xmsg& operator<<(const uint64_t& v)
      {
      return prt(L"%08X%08X", (uint32_t)(v >> (CHAR_BIT * sizeof(uint32_t))), (uint32_t)v);
      }
    /// 输出 hex 指针。
    xmsg& operator<<(const void* const v)
      {
      return operator<<((size_t)v);
      }
    /// 输出 :true :false。
    xmsg& operator<<(const bool& v)
      {
      return operator<<(v ? L":true" : L":false");
      }
    /// 输出 ANSI 字符 转换 UNICCODE 字符。
    xmsg& operator<<(const char& v)
      {
      append(as2ws(std::string(1, v)));
      return *this;
      }
    /// 输出 ANSI 字符串 转换 UNICCODE 字符串。
    xmsg& operator<<(const char* const v)
      {
      if(nullptr != v) append(as2ws(v));
      return *this;
      }
    /// 输出 ASNI 字符串 转换 UNICCODE 字符串。
    xmsg& operator<<(const std::string& v)
      {
      append(as2ws(v));
      return *this;
      }
    /// 输出 UNICCODE 字符。
    xmsg& operator<<(const wchar_t& v)
      {
      append(1, v);
      return *this;
      }
    /// 输出 UNICCODE 字符串。
    xmsg& operator<<(const wchar_t* const v)
      {
      if(nullptr != v) append(v);
      return *this;
      }
    /// 输出 UNICCODE 字符串。
    xmsg& operator<<(const std::wstring& v)
      {
      append(v);
      return *this;
      }
    /// 输出 UTF-8 转换 UNICCODE 字符。
    xmsg& operator<<(const char8_t& v)
      {
      append(u82ws(std::u8string(1, v)));
      return *this;
      }
    /// 输出 UTF-8 字符串 转换 UNICCODE 字符串。
    xmsg& operator<<(const char8_t* v)
      {
      if(nullptr != v) append(u82ws(v));
      return *this;
      }
    /// 输出 UTF-8 字符串 转化 UNICCODE 字符串。
    xmsg& operator<<(const std::u8string& v)
      {
      append(u82ws(v));
      return *this;
      }
    /// 输出 dec 浮点数。
    xmsg& operator<<(const float& v)
      {
      return prt(L"%f", v);
      }
    /// 输出 dec 浮点数。
    xmsg& operator<<(const double& v)
      {
      return prt(L"%f", v);
      }
    /// 输出 内容。
    xmsg& operator<<(const xmsg& v)
      {
      append(v);
      return *this;
      }
  };

#endif  // _XLIB_XMSG_H_