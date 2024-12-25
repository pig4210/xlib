/**
  \file  xcodecvt.h
  \brief 用于 ANSI 与 UNICODE 及 UTF8 等编码的本地化转换。

  \version    0.0.1.241126

  \author     triones
  \date       2019-08-02

  \section more 额外说明

  - linux 下，注意查询本地化支持： `locale -a` （否则，会在 `std::locale` 时崩溃）。
      - 安装 GB2312 ： `sudo locale-gen zh_CN` 。
      - 安装 UTF8 中文： `sudo apt-get install language-pack-zh-hans` 。
  - g++ 默认编码为 UTF8 ，如需以 ANSI 编译，需加入编译参数如： `-fexec-charset=GB2312` 。
  - 需要 gcc-9.2.0 及以上支持。

  \section history 版本记录

  - 2024-11-26 重建 。 0.0.1 。
*/
#ifndef _XLIB_XCODECVT_H_
#define _XLIB_XCODECVT_H_

#include <string>

// 不支持 chat8_t 时，自制 char8_t 。
#ifndef __cpp_char8_t
using char8_t = unsigned char;
#define XTEXT(__s) (const char8_t*)u8 ## __s
#define XCHAR(__c) (char8_t)u8 ## __c
#else // __cpp_char8_t
#define XTEXT(__s) u8 ## __s
#define XCHAR(__c) u8 ## __c
#endif // __cpp_char8_t

// 不支持 std::u8string 时，自制 std::u8string 。
#ifndef __cpp_lib_char8_t
// 违反规则，添加进 std ，以模拟实现。
namespace std {
using u8string = basic_string<char8_t, char_traits<char8_t>, allocator<char8_t>>;
using u8string_view = basic_string_view<char8_t, char_traits<char8_t>>;
}
#endif

namespace xlib {
// 因为 isprint 与 isspace 不是 constexpr ，所以自制。
constexpr bool inline is_easy_transcoding(const unsigned long c) {
  return (c >= 0x20 && c <= 0x7E) || (c >= 0x9 && c <= 0xD);
}
constexpr bool inline is_easy_transcoding(const char& c) {
  return is_easy_transcoding((unsigned long)c);
}
constexpr bool inline is_easy_transcoding(const char8_t& c) {
  return is_easy_transcoding((unsigned long)c);
}
constexpr bool inline is_easy_transcoding(const wchar_t& c) {
  return is_easy_transcoding((unsigned long)c);
}
}  // namespace xlib

// 如果有 iconv.h ，则使用 iconv 。
#if __has_include(<iconv.h>)
#include "xcodecvt_iconv.h"
#else
// VS2017 对 codecvt 有各种局限，为了方便，干脆限制之，而使用 xcodecvt_win.h 。
#if defined(_WIN32) && (_MSVC_LANG <= 202002L)
#include "xcodecvt_win.h"
#else
#include "xcodecvt_std.h"
#endif
#endif

#endif  // _XLIB_XCODECVT_H_