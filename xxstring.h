/**
  \file  xxstring.h
  \brief 对于 存放 UTF-8 编码的 std::string ，用 xxstring 替换会方便一些。

  \version    0.0.2.241230

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
    std::u8string::operator=(std::move(s));
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
  // 返回引用，强转，而不进行编码转换。
  const std::string& beas() const { return *(const std::string*)this; }
  std::string& beas() { return *(std::string*)this; }
  std::string toas() const { return *(const std::string*)this; }
  operator const std::string&() const { return *(const std::string*)this; }
  operator std::string&() { return *(std::string*)this; }
  std::string&& move_as() { return std::move(*(std::string*)this); }
  /*
    下面的一些转换已经尝试过，但都存在一些冲突。
    其中， 不能与上面共存，编译无冲突，但使用有冲突。
    explicit std::string&&() 不冲突了，但 move 无效。
    实在需要 move 时，使用 move_as() 。
    注：std::move<std::string> 与 std::move<std::string&&> 都不行。
  */
  //operator std::string() const & { return *(const std::string*)this; }
  //operator const std::string() & { return *(const std::string*)this; }
  //operator std::string&&() { return std::move(*(std::string*)this); }
  // 转换成 std::wstring ，有编码转换。
  std::wstring tows() const { return xlib::u82ws(*this); }
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

  const xlib::xmsg& bexmsg() const { return *(const xlib::xmsg*)this; }
  xlib::xmsg& bexmsg() { return *(xlib::xmsg*)this; }
  xlib::xmsg tomsg() const { return *(const xlib::xmsg*)this; }
  operator const xlib::xmsg&() const { return *(const xlib::xmsg*)this; }
  operator xlib::xmsg&() { return *(xlib::xmsg*)this; }
  xlib::xmsg&& move_xmsg() { return std::move(*(xlib::xmsg*)this); }

 public:
  // 扩展支持 xmsg 输出。好像不需要。因为基类是 std::u8string 。
  //friend xmsg& operator<<(xmsg& msg, const xxstring& s) {
  //  return msg << std::u8string(s);
  //}
};

}  // namespace xlib

#endif  // _XLIB_XXSTRING_H_