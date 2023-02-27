/**
  \file  xmsg.h
  \brief 定义了信息组织的基本类，类似标准库的 ostreamstring 。

  \version    5.1.3.230224
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
  - 2019-11-03 引入 char8_t 、 u8string 。 2.1 。
  - 2020-03-09 改变基类为 wstring 。 3.0 。
  - 2020-11-12 改变基类可选，改变数值输出为模板 。 4.0 。
  - 2021-06-20 基类固定为 u8string 。 5.0 。
  - 2022-01-05 向下兼容 c++17 。
*/
#ifndef _XLIB_XMSG_H_
#define _XLIB_XMSG_H_

#include <climits>
#include <cstdarg>
#include <cstdio>
#include <string>

#include "xcodecvt.h"

namespace xlib {

#ifdef XLIB_NOCXX20
#define XMSGT(__t) (const char8_t*)u8 ## __t
#else
#define XMSGT(__t) u8 ## __t
#endif
#define XMSGAS(__v) xlib::as2u8(__v)
#define XMSGWS(__v) xlib::ws2u8(__v)
#define XMSGU8(__v) __v

class xmsg : public std::u8string {
 public:
  using std::u8string::u8string;

 public:
  xmsg() {}
  xmsg(const std::string& as)   : std::u8string(XMSGAS(as)) {}
  xmsg(const std::wstring& ws)  : std::u8string(XMSGWS(ws)) {}
  xmsg(const std::u8string& u8) : std::u8string(XMSGU8(u8)) {}

 public:
  /// 指定格式输出。
  xmsg& prt(const char8_t* const fmt, ...) {
    if (nullptr == fmt) return *this;
    va_list ap;
    va_start(ap, fmt);
    const auto need = std::vsnprintf(nullptr, 0, (const char*)fmt, ap);
    va_end(ap);
    if (0 >= need) return *this;
    std::u8string buffer;
    buffer.resize(need);
    va_start(ap, fmt);
    std::vsnprintf((char*)buffer.data(), buffer.size() + 1, (const char*)fmt, ap);
    va_end(ap);
    append(buffer);
    return *this;
  }
  /// 针对 UTF-8 对齐问题， %s 格式化建议使用 char* 。
  xmsg& prt(const char* const fmt, ...) {
    if (nullptr == fmt) return *this;
    va_list ap;
    va_start(ap, fmt);
    const auto need = std::vsnprintf(nullptr, 0, (const char*)fmt, ap);
    va_end(ap);
    if (0 >= need) return *this;
    std::string buffer;
    buffer.resize(need);
    va_start(ap, fmt);
    std::vsnprintf(buffer.data(), buffer.size() + 1, (const char*)fmt, ap);
    va_end(ap);
    append(XMSGAS(buffer));
    return *this;
  }
  /// 输出 dec 值。
  template <typename T> std::enable_if_t<sizeof(T) == sizeof(int8_t) && std::is_signed_v<T>, xmsg&>
  operator<<(const T& v) {
    return prt(XMSGT("%hhi"), v);
  }
  /// 输出 hex(XX)。
  template <typename T> std::enable_if_t<sizeof(T) == sizeof(uint8_t) && (std::is_unsigned_v<T> || std::is_enum_v<T>), xmsg&>
  operator<<(const T& v) {
    return prt(XMSGT("%02X"), v);
  }
  /// 输出 dec 值。
  template <typename T> std::enable_if_t<sizeof(T) == sizeof(int16_t) && std::is_signed_v<T>, xmsg&>
  operator<<(const T& v) {
    return prt(XMSGT("%hi"), v);
  }
  /// 输出 hex(XXXX)。
  template <typename T> std::enable_if_t<sizeof(T) == sizeof(uint16_t) && (std::is_unsigned_v<T> || std::is_enum_v<T>), xmsg&>
  operator<<(const T& v) {
    return prt(XMSGT("%04X"), v);
  }
  /// 输出 dec 值。
  template <typename T> std::enable_if_t<sizeof(T) == sizeof(int32_t) && std::is_signed_v<T>, xmsg&>
  operator<<(const T& v) {
    return prt(XMSGT("%i"), v);
  }
  /// 输出 hex(XXXXXXXX)。
  template <typename T> std::enable_if_t<sizeof(T) == sizeof(uint32_t) && (std::is_unsigned_v<T> || std::is_enum_v<T>), xmsg&>
  operator<<(const T& v) {
    return prt(XMSGT("%08X"), v);
  }
  /// 输出 dec 值。
  template <typename T> std::enable_if_t<sizeof(T) == sizeof(int64_t) && std::is_signed_v<T>, xmsg&>
  operator<<(const T& v) {
    return prt(XMSGT("%lli"), v);
  }
  /// 输出 hex(XXXXXXXXXXXXXXXX)。
  template <typename T> std::enable_if_t<sizeof(T) == sizeof(uint64_t) && (std::is_unsigned_v<T> || std::is_enum_v<T>), xmsg&>
  operator<<(const T& v) {
    return prt(XMSGT("%08X%08X"), (uint32_t)(v >> (CHAR_BIT * sizeof(uint32_t))), (uint32_t)v);
  }
  /// 输出 hex 指针。
  xmsg& operator<<(const void* const v) {
    return operator<<((uintptr_t)v);
  }
  /// 输出 true 或 false。
  xmsg& operator<<(const bool& v) {
    return operator<<(v ? XMSGT("true") : XMSGT("false"));
  }
  /// 输出 ANSI 字符 转换。
  xmsg& operator<<(const char& v) {
    append(XMSGAS(std::string(1, v)));
    return *this;
  }
  /// 输出 ANSI 字符串 转换。
  xmsg& operator<<(const char* const v) {
    if (nullptr != v) append(XMSGAS(v));
    return *this;
  }
  /// 输出 ASNI 字符串 转换。
  xmsg& operator<<(const std::string& v) {
    append(XMSGAS(v));
    return *this;
  }
  /// 输出 UNICCODE 字符 转换。
  xmsg& operator<<(const wchar_t& v) {
    append(XMSGWS(std::wstring(1, v)));
    return *this;
  }
  /// 输出 UNICCODE 字符串 转换。
  xmsg& operator<<(const wchar_t* const v) {
    if (nullptr != v) append(XMSGWS(v));
    return *this;
  }
  /// 输出 UNICCODE 字符串 转换。
  xmsg& operator<<(const std::wstring& v) {
    append(XMSGWS(v));
    return *this;
  }
#ifndef XLIB_NOCXX20
  /// 输出 UTF-8 字符 转换。
  xmsg& operator<<(const char8_t& v) {
    append(XMSGU8(std::u8string(1, v)));
    return *this;
  }
#endif
  /// 输出 UTF-8 字符串 转换。
  xmsg& operator<<(const char8_t* v) {
    if (nullptr != v) append(XMSGU8(v));
    return *this;
  }
  /// 输出 UTF-8 字符串 转换。
  xmsg& operator<<(const std::u8string& v) {
    append(XMSGU8(v));
    return *this;
  }
  /// 输出 dec 浮点数。
  xmsg& operator<<(const float& v) {
    return prt(XMSGT("%f"), v);
  }
  /// 输出 dec 浮点数。
  xmsg& operator<<(const double& v) {
    return prt(XMSGT("%f"), v);
  }
  /// 输出 内容。
  xmsg& operator<<(const xmsg& v) {
    append(v);
    return *this;
  }
  /// 转换 ASCII 。
  std::string toas() const {
    return u82as(*this);
  }
  /// 转换 UNICODE 。
  std::wstring tows() const {
    return u82ws(*this);
  }
};

#undef XMSGT
#undef XMSGAS
#undef XMSGWS
#undef XMSGU8

}  // namespace xlib

#endif  // _XLIB_XMSG_H_