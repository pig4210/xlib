/**
  \file  xxstring.h
  \brief 对于 存放 UTF-8 编码的 std::string ，用 xxstring 替换会方便一些。

  \version    0.0.1.230317

  \author     triones
  \date       2023-03-17

  \section history 版本记录

  - 2023-03-17 新建 xxstring 类。
*/
#ifndef _XLIB_XXSTRING_H_
#define _XLIB_XXSTRING_H_

#include "xcodecvt.h"
#include "xmsg.h"

namespace xlib {

class xxstring : public std::u8string {
 public:
  // 开放 std::u8string 构造。 其他有默认构造。
  using std::u8string::u8string;
 public:
  xxstring() = default;
  xxstring(const xxstring&) = default;
  xxstring& operator=(const xxstring&) = default;
  xxstring(xxstring&&) = default;
  xxstring& operator=(xxstring&&) = default;

 public:
  xxstring(const std::u8string& s) : std::u8string(s) {}
  xxstring& operator=(const std::u8string& s) {
    assign(s);
    return *this;
  }
  xxstring(std::u8string&& s) : std::u8string(std::move(s)) {}
  xxstring& operator=(std::u8string&& s) {
    operator=(std::move(s));
    return *this;
  }

 public:
  // std::string 构造，视之为 UTF-8 编码，不进行编码转换。
  xxstring(const std::string& s): xxstring(*(const xxstring*)&s) {}
  xxstring& operator=(const std::string& s) {
    return operator=(*(const xxstring*)&s);
  }
  xxstring(std::string&& s): xxstring(std::move(*(xxstring*)&s)) {}
  xxstring& operator=(std::string&& s) {
    return operator=(std::move(*(xxstring*)&s));
  }
  
 public:
  // std::wstring 构造。无法实现 移动语义。
  xxstring(const std::wstring& s): std::u8string(xlib::ws2u8(s)) {}
  xxstring& operator=(const std::wstring& s) {
    std::u8string::operator=(xlib::ws2u8(s));
    return *this;
  }

 public:
  // 转换成 std::string ，强转，而不进行编码转换。
  operator std::string() const { return *(const std::string*)this; }
  // 转换成 std::wstring ，有编码转换。
  operator std::wstring() const { return xlib::u82ws(*this); }

 public:
  bool operator==(const xxstring& v) {
    return *(const std::u8string*)(this) == *(const std::u8string*)(&v);
  }
  bool operator!=(const xxstring& v) { return !operator==(v); }

 public:
  // 扩展支持 xmsg 构造。
  xxstring(const xlib::xmsg& s) : std::u8string(s) {}
  xxstring& operator=(const xlib::xmsg& s) { return operator=(s); }
  xxstring(xlib::xmsg&& s): std::u8string(std::move(s)) {}
  xxstring& operator=(xlib::xmsg&& s) { return operator=(std::move(s)); }
};

}  // namespace xlib

/// 扩展支持 xmsg 。
inline xlib::xmsg& operator<<(xlib::xmsg& msg, const xlib::xxstring& s) {
  return msg << std::u8string(s);
}

#endif  // _XLIB_XRAND_H_