/**
  \file  xmsg.h
  \brief 定义了信息组织的基本类，类似标准库的 ostreamstring 。

  \version    5.0.0.201112
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
  - 2020-11-12 改变基类可选，改变数值输出为模板 。 5.0 。
*/
#ifndef _XLIB_XMSG_H_
#define _XLIB_XMSG_H_

#include <string>
#include <cstdio>
#include <cstdarg>
#include <climits>

#include "xcodecvt.h"

#define XMSG_BASE_ANSI    0
#define XMSG_BASE_UNICODE 1
#define XMSG_BASE_UTF8    2

// 未定义时，默认 windows 使用 ANSI 编码，unix 使用 UNICODE 编码。
#ifndef XMSG_BASE
  #ifdef _WIN32
    #define XMSG_BASE XMSG_BASE_ANSI
  #else
    #define XMSG_BASE XMSG_BASE_UNICODE
  #endif
#endif

#if XMSG_BASE == XMSG_BASE_UTF8
  using xmsg_base = std::u8string;
  #define XMSG_TEXT(s) u8##s
  #define xmsg_base_as(as) xmsg_base(as2u8(as))
  #define xmsg_base_ws(ws) xmsg_base(ws2u8(ws))
  #define xmsg_base_u8(u8) xmsg_base(u8)
#elif XMSG_BASE == XMSG_BASE_UNICODE
  using xmsg_base = std::wstring;
  #define XMSG_TEXT(s) L##s
  #define xmsg_base_as(as) xmsg_base(as2ws(as))
  #define xmsg_base_ws(ws) xmsg_base(ws)
  #define xmsg_base_u8(u8) xmsg_base(u82ws(u8))
#else
  using xmsg_base = std::string;
  #define XMSG_TEXT(s) s
  #define xmsg_base_as(as) xmsg_base(as)
  #define xmsg_base_ws(ws) xmsg_base(ws2as(ws))
  #define xmsg_base_u8(u8) xmsg_base(u82as(u8))
#endif

class xmsg : public xmsg_base
  {
  public:
    xmsg() {}
    xmsg(const std::string& as):xmsg_base_as(as) {}
    xmsg(const std::wstring& ws):xmsg_base_ws(ws) {}
    xmsg(const std::u8string& u8):xmsg_base_u8(u8) {}
  public:
    /// 指定格式输出。
#if XMSG_BASE == XMSG_BASE_UTF8
    xmsg& prt(const char8_t* const fmt, ...)
      {
      if(nullptr == fmt) return *this;
      va_list ap;
      va_start(ap, fmt);
      const auto need = std::vsnprintf(nullptr, 0, (const char*)fmt, ap);
      va_end(ap);
      if(0 >= need) return *this;
      xmsg_base buffer;
      buffer.resize(need);
      va_start(ap, fmt);
      std::vsnprintf((char*)buffer.data(), buffer.size() + 1, (const char*)fmt, ap);
      va_end(ap);
      append(buffer);
      return *this;
      }
#elif XMSG_BASE == XMSG_BASE_UNICODE
    xmsg& prt(const wchar_t* const fmt, ...)
      {
      if(nullptr == fmt) return *this;
      va_list ap;
      for(size_t rest = wcslen(fmt) * sizeof(wchar_t); rest; rest *= sizeof(wchar_t))
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
#else
    xmsg& prt(const char* const fmt, ...)
      {
      if(nullptr == fmt) return *this;
      va_list ap;
      va_start(ap, fmt);
      const auto need = std::vsnprintf(nullptr, 0, fmt, ap);
      va_end(ap);
      if(0 >= need) return *this;
      std::string buffer;
      buffer.resize(need);
      va_start(ap, fmt);
      std::vsnprintf(buffer.data(), buffer.size() + 1, fmt, ap);
      va_end(ap);
      append(buffer);
      return *this;
      }
#endif
    /// 输出 dec 值。
    template<typename T> std::enable_if_t<std::is_signed_v<T> && sizeof(T) == sizeof(int8_t), xmsg&> operator<<(const T& v)
      {
      return prt(XMSG_TEXT("%hhi"), v);
      }
    /// 输出 hex(XX)。
    template<typename T> std::enable_if_t<std::is_unsigned_v<T> && sizeof(T) == sizeof(uint8_t), xmsg&> operator<<(const T& v)
      {
      return prt(XMSG_TEXT("%02X"), v);
      }
    /// 输出 dec 值。
    template<typename T> std::enable_if_t<std::is_signed_v<T> && sizeof(T) == sizeof(int16_t), xmsg&> operator<<(const T& v)
      {
      return prt(XMSG_TEXT("%hi"), v);
      }
    /// 输出 hex(XXXX)。
    template<typename T> std::enable_if_t<std::is_unsigned_v<T> && sizeof(T) == sizeof(uint16_t), xmsg&> operator<<(const T& v)
      {
      return prt(XMSG_TEXT("%04X"), v);
      }
    /// 输出 dec 值。
    template<typename T> std::enable_if_t<std::is_signed_v<T> && sizeof(T) == sizeof(int32_t), xmsg&> operator<<(const T& v)
      {
      return prt(XMSG_TEXT("%i"), v);
      }
    /// 输出 hex(XXXXXXXX)。
    template<typename T> std::enable_if_t<std::is_unsigned_v<T> && sizeof(T) == sizeof(uint32_t), xmsg&> operator<<(const T& v)
      {
      return prt(XMSG_TEXT("%08X"), v);
      }
    /// 输出 dec 值。
    template<typename T> std::enable_if_t<std::is_signed_v<T> && sizeof(T) == sizeof(int64_t), xmsg&> operator<<(const T& v)
      {
      return prt(XMSG_TEXT("%lli"), v);
      }
    /// 输出 hex(XXXXXXXXXXXXXXXX)。
    template<typename T> std::enable_if_t<std::is_unsigned_v<T> && sizeof(T) == sizeof(uint64_t), xmsg&> operator<<(const T& v)
      {
      return prt(XMSG_TEXT("%08X%08X"), (uint32_t)(v >> (CHAR_BIT * sizeof(uint32_t))), (uint32_t)v);
      }
    /// 输出 hex 指针。
    xmsg& operator<<(const void* const v)
      {
      return operator<<((size_t)v);
      }
    /// 输出 :true :false。
    xmsg& operator<<(const bool& v)
      {
      return operator<<(v ? XMSG_TEXT(":true") : XMSG_TEXT(":false"));
      }
    /// 输出 ANSI 字符 转换。
    xmsg& operator<<(const char& v)
      {
      append(xmsg_base_as(std::string(1, v)));
      return *this;
      }
    /// 输出 ANSI 字符串 转换。
    xmsg& operator<<(const char* const v)
      {
      if(nullptr != v) append(xmsg_base_as(v));
      return *this;
      }
    /// 输出 ASNI 字符串 转换。
    xmsg& operator<<(const std::string& v)
      {
      append(xmsg_base_as(v));
      return *this;
      }
    /// 输出 UNICCODE 字符 转换。
    xmsg& operator<<(const wchar_t& v)
      {
      append(xmsg_base_ws(std::wstring(1, v)));
      return *this;
      }
    /// 输出 UNICCODE 字符串 转换。
    xmsg& operator<<(const wchar_t* const v)
      {
      if(nullptr != v) append(xmsg_base_ws(v));
      return *this;
      }
    /// 输出 UNICCODE 字符串 转换。
    xmsg& operator<<(const std::wstring& v)
      {
      append(xmsg_base_ws(v));
      return *this;
      }
    /// 输出 UTF-8 字符 转换。
    xmsg& operator<<(const char8_t& v)
      {
      append(xmsg_base_u8(std::u8string(1, v)));
      return *this;
      }
    /// 输出 UTF-8 字符串 转换。
    xmsg& operator<<(const char8_t* v)
      {
      if(nullptr != v) append(xmsg_base_u8(v));
      return *this;
      }
    /// 输出 UTF-8 字符串 转换。
    xmsg& operator<<(const std::u8string& v)
      {
      append(xmsg_base_u8(v));
      return *this;
      }
    /// 输出 dec 浮点数。
    xmsg& operator<<(const float& v)
      {
      return prt(XMSG_TEXT("%f"), v);
      }
    /// 输出 dec 浮点数。
    xmsg& operator<<(const double& v)
      {
      return prt(XMSG_TEXT("%f"), v);
      }
    /// 输出 内容。
    xmsg& operator<<(const xmsg& v)
      {
      append(v);
      return *this;
      }
  };

#endif  // _XLIB_XMSG_H_