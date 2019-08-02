/**
  \file  ws_s.h
  \brief 用于 ASCII 与 UNICODE(UCS2) 相互转换。

  \version    1.3.0.161223
  \note       For All

  \author     triones
  \date       2011-11-11
  
  \section more 额外说明

  - 使用内核函数转换，而不是使用转换 API 。
  - Windows 使用系统默认的 ASCII 编码（一般是 GB2312 ）。
  - Linux 需要设置，默认 GB2312 。（使用 ICONV 库。）
  - 由于 Windows 与 Linux 的 Unicode 默认不同，所以这里指的 Unicode 统一为 UCS2 。
  - Linux 下即使编译选项 -fshort-wchar 也不能用 wchar_t 和 wstring ，会产生莫名问题。
  - 注意：Windows 下处理 ASCII 时，当字串长度超过 0x7FFE 且刚好字符截断，后继转换会有问题。

  \section history 版本记录

  - 2011-12-29 新建此模块专门处理 ws&s 转换。
  - 2011-12-29 处理 ws&s 的一个隐藏 BUG 。重新调整 ws&s 的设计。 0.1 。
  - 2012-05-24 重新设计 ws&s ，使 Ring0 与 Ring3 代码通用。 0.2 。
  - 2012-05-24 同时发现原来对 UNICODE_STRING 的一个认识错误，修正 BUG 。
  - 2012-05-24 as & us.Length 表示实际字符串占用的 byte ，不包含结尾 null ，但转换时生成 null 。
  - 2012-06-04 处理流程优化。 0.2.1 。
  - 2012-09-25 发现 s2ws 处理结尾 null 上的错误，修正之。 1.0 。
  - 2013-09-25 去除转换为标准库的重载。 1.1 。
  - 2014-01-11 引入 SIGSTL ，修改适应标准库。 1.2 。
  - 2014-04-17 细节改善。 1.2.1 。
  - 2014-05-07 细节改进。 1.2.2 。
  - 2014-08-26 改进使转换的完整。 1.2.3 。
  - 2015-05-09 修正输入数据过大导致转换不完全的BUG。 1.2.4 。
  - 2015-06-13 修正上一版本产生的转换 BUG 。 1.2.5 。
  - 2016-03-14 修正 s2ws 处理给定 ws 缓冲计算错误的 BUG 。 1.2.6 。
  - 2016-12-23 适配 Linux g++ 。简化接口。 1.3 。
*/
#ifndef _XLIB_WS_S_H_
#define _XLIB_WS_S_H_

#include <string>

#include "xlib_base.h"

#ifdef _WIN32
#   define charucs2_t    wchar_t
#   define ucs2string    std::wstring
#else
#   define charucs2_t    char16_t
#   define ucs2string    std::u16string
#endif

const char cnull = '\0';
const charucs2_t snull = L'\0';

/**
  UNICODE 串转换 ASCII 串。
  \param  ws    需要转换的 UNICODE 串。
  \param  size  需要转换的 UNICODE 串的长度（以宽字计）。
  \return       转换后的对应 ASCII 串对象。

  \code
    std::cout << ws2s(L"文字");
  \endcode
*/
std::string ws2s(const charucs2_t* const ws, const size_t size);
std::string ws2s(const ucs2string& ws);
 
/**
  ASCII 串转换 UNICODE 串。
  \param  s     需要转换的 ASCII 串
  \param  size  需要转换的 ASCII 串的长度。
  \return       转换后的对应 UNICODE 串对象。

  \code
    std::wcout << s2ws("文字");
  \endcode
  */
ucs2string s2ws(const char* const s, const size_t size);
ucs2string s2ws(const std::string& s);

#ifndef _WIN32
/**
  设置 ASCII 默认编码
  \param  encode  需要改变的默认编码。\n
                  当 encode == nullptr 时，返回当前默认编码。\n
                  当 encode == "" 时，设置默认编码为 GB2312 。
  \return         设置后的编码。
*/
const char* set_ascii_encode(const char* encode = nullptr);

#endif

#endif  // _XLIB_WS_S_H_