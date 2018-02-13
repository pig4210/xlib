/*!
  \file   xlib_link.h
  \brief  为静态库提供自动链接机制。

  - 可以根据当前工程的编译选项连接到正确版本的静态库。
  - 具体作用请联系作者，或是参考boost库的自动链接机制。
  - xlib库自身并不使用它
  - 支持Ring0、Ring3下的MT、MTd、MD、MDd运行时库
  - \b 修改 by \b triones

  \code
    #include "xlib_link.h" //根据当前工程的编译选项连接到相应静态库。
    #pragma comment(lib,"Work\\NewLIB\\NewLIB"AUTOLINK_VER)   //自动链接
  \endcode

  \version    4.1.1711.1517
  \note       For All

  \author   uni
  \author   triones
  \date     2011-4-8
*/
#ifndef _XIB_LINK_H_
#define _XIB_LINK_H_

//! linux和xlib自身不需要自动链接
#if defined(_WIN32) && !defined(XLIB_ALREADY_LINK)

#define XLIB_ALREADY_LINK

#if defined(__MSVC_RUNTIME_CHECKS) && !defined(_DEBUG)
#    pragma message("使用 /RTC 选项而不指定一个调试版本的运行时库会导致连接错误")
#    pragma message("提示： 在代码生成中选择一个调试版本的运行时库")
#    error "不兼容的编译选项"
#endif

//! AUTO_LINK_RUNTIME_NAME = [ "MDd"|"MD"|"MTd"|"MT" ]; 用以表示运行时库
#ifdef _DLL
#    ifdef _DEBUG
#        define AUTO_LINK_RUNTIME_NAME "MDd"
#    else
#        define AUTO_LINK_RUNTIME_NAME "MD"
#    endif
#else
#    ifdef _DEBUG
#        define AUTO_LINK_RUNTIME_NAME "MTd"
#    else
#        define AUTO_LINK_RUNTIME_NAME "MT"
#    endif
#endif

//! AUTOLINK_CPU_RING = [ "0"|"3" ]; 用以表示Ring0/Ring3
#ifdef FOR_RING0
#    define AUTOLINK_CPU_RING  "0"
#else
#    define AUTOLINK_CPU_RING  "3"
#endif

//! AUTOLINK_X86_64 = [ ""| "x64" ]; 用以表示x86/x64，注意x86时为空串
#ifdef _WIN64
#    define AUTOLINK_X86_64 "x64"
#else
#    define AUTOLINK_X86_64 ""
#endif

//! AUTOLINK_PLATFORM = [ "x86"| "x64" ]; 用以表示x86/x64
#ifdef _WIN64
#    define AUTOLINK_PLATFORM   "x64"
#else
#    define AUTOLINK_PLATFORM   "x86"
#endif

#define AUTOLINK_VER AUTO_LINK_RUNTIME_NAME AUTOLINK_CPU_RING AUTOLINK_X86_64

#pragma comment(lib, "xlib_" AUTOLINK_VER)

#endif  // _WIN32  !XLIB_ALREADY_LINK

#endif  // _XIB_LINK_H_