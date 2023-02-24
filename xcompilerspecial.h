#ifndef _XLIB_XCOMPILERSPECIAL_H_
#define _XLIB_XCOMPILERSPECIAL_H_

// VS2017 无法使用 c++20 。
#if defined(_WIN32) || defined(_WIN64)

#if _MSVC_LANG <= 202002L
#define XLIB_NOCXX20
#endif

#endif  // _WIN32 || _WIN64

#ifdef XLIB_NOCXX20

#include <string>
// 有 BUG ！！！需要在某个 CPP 定义如下注释代码。
// std::locale::id std::codecvt<char16_t, char, _Mbstatet>::id;

using char8_t = unsigned char;

// 违反规则，添加进 std ，以模拟实现。
namespace std {
using u8string = basic_string<char8_t, char_traits<char8_t>, allocator<char8_t>>;
}

#define XTEXT(__s) (const char8_t*)u8 ## __s
#define XCHAR(__c) (char8_t)u8 ## __c

#else  // XLIB_NOCXX20

#define XTEXT(__s) u8 ## __s
#define XCHAR(__c) u8 ## __c

#endif

#endif  //_XLIB_XCOMPILERSPECIAL_H_