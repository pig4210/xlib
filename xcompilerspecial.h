#ifndef _XLIB_XCOMPILERSPECIAL_H_
#define _XLIB_XCOMPILERSPECIAL_H_

#if defined(_WIN32) || defined(_WIN64)
#include <string>

#if _MSVC_LANG <= 202002L
#define XLIB_NOCXX20
#endif

#ifdef XLIB_NOCXX20
// 有 BUG ！！！需要在某个 CPP 定义如下注释代码。
// std::locale::id std::codecvt<char16_t, char, _Mbstatet>::id;

using char8_t = unsigned char;

// 违反规则，添加进 std ，以模拟实现。
namespace std
  {
  using u8string = basic_string<char8_t, char_traits<char8_t>, allocator<char8_t>>;
  }
#endif
#endif  // _WIN32 || _WIN64

#define XTEXT(s) (const char8_t*)u8 ## s
#define XCHAR(c) (char8_t)u8 ## c

#endif  //_XLIB_XCOMPILERSPECIAL_H_