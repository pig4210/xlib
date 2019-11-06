﻿/**
  \file  as_ws_u8.h
  \brief 用于 ANSI 与 UNICODE 及 UTF8 等编码的本地化转换。

  \version    2.0.0.191103

  \author     triones
  \date       2019-08-02

  \section more 额外说明

  - linux 下，注意查询本地化支持： `locale -a` 。
      - 安装 GB2312 ： `sudo locale-gen zh_CN` 。
      - 安装 UTF8 中文： `sudo apt-get install language-pack-zh-hans` 。
  - g++ 默认编码为 UTF8 ，如需以 ANSI 编译，需加入编译参数如： `-fexec-charset=GB2312` 。
  - 需要 gcc-9.2.0 及以上支持。

  \section history 版本记录

  - 2019-08-05 新建。 0.1 。
  - 2019-09-25 使用 codecvt 重构。 1.0 。
  - 2019-11-03 引入 u8string ，再次重构。 2.0 。
*/
#ifndef _XLIB_AS_WS_U8_H_
#define _XLIB_AS_WS_U8_H_

#include <string>
#include <locale>

/// 允许通过设置 LOCALE_AS_WS 宏，改变默认 ANSI 编码。
#ifndef LOCALE_AS_WS
#ifdef _WIN32
#define LOCALE_AS_WS   ("zh-CN")
#else
#define LOCALE_AS_WS   ("zh_CN.GB2312")
#endif
#endif

/// 允许通过设置 LOCALE_WS_U8 宏，改变默认 UTF8 编码。
#ifndef LOCALE_WS_U8
#ifdef _WIN32
#define LOCALE_WS_U8  ("zh-CN.UTF8")
#else
#define LOCALE_WS_U8  ("zh_CN.UTF8")
#endif
#endif

using XCODECVT = std::codecvt<wchar_t, char, std::mbstate_t>;

/**
  ANSI 串转换 UNICODE 串。
  \param  as    需要转换的 ANSI 串
  \return       转换后的对应 UNICODE 串。

  \code
    auto ws = as2ws("文字");
  \endcode
*/
inline std::wstring as2ws(const std::string& as, size_t* const readed = nullptr)
  {
  if(as.empty())  return std::wstring();

  std::locale locale(LOCALE_AS_WS);

  std::mbstate_t state{};

  size_t rd;
  size_t* const lpread = (nullptr == readed) ? &rd : readed;
  *lpread = 0;

  auto& cvt = std::use_facet<XCODECVT>(locale);

  const auto need = as.size();

  std::wstring ws;
  ws.resize(need, L'\0');

  const char* from        = as.data();
  const char* from_end    = from + as.size();
  const char* from_next;
  
  wchar_t* to             = ws.data();
  wchar_t* to_end         = to + ws.size();
  wchar_t* to_next;

  const auto result = cvt.in(
    state,
    from,   from_end,   from_next,
    to,     to_end,     to_next);

  *lpread = from_next - from;
  ws.resize(to_next - to);

  return (XCODECVT::ok != result && nullptr == readed) ? std::wstring() : ws;
  }

/**
  UNICODE 串转换 ANSI 串。
  \param  ws    需要转换的 UNICODE 串。
  \return       转换后的对应 ANSI 串。

  \code
    auto as = ws2as(L"文字");
  \endcode
*/
inline std::string ws2as(const std::wstring& ws, size_t* const readed = nullptr)
  {
  if(ws.empty())  return std::string();

  std::locale locale(LOCALE_AS_WS);

  std::mbstate_t state{};

  size_t rd;
  size_t* lpread = (nullptr == readed) ? &rd : readed;
  *lpread = 0;

  auto& cvt = std::use_facet<XCODECVT>(locale);

  const auto need = ws.size() * cvt.max_length();
  
  std::string as;
  as.resize(need, '\0');

  const wchar_t* from       = ws.data();
  const wchar_t* from_end   = from + ws.size();
  const wchar_t* from_next;

  char* to                  = as.data();
  char* to_end              = to + as.size();
  char* to_next;

  const auto result = cvt.out(
    state,
    from,   from_end,   from_next,
    to,     to_end,     to_next);

  *lpread = from_next - from;
  as.resize(to_next - to);

  return (XCODECVT::ok != result && nullptr == readed) ? std::string() : as;
  }

/**
  UTF8 串转换 UNICODE 串。
  \param    u8    需要转换的 UTF8 串。
  \return         转换后的对应 UNICODE 串。

  \code
    auto ws(u82ws(u8"文字"));
  \endcode
*/
inline std::wstring u82ws(const std::u8string& u8, size_t* const readed = nullptr)
  {
  if(u8.empty())  return std::wstring();

  std::locale locale(LOCALE_WS_U8);

  std::mbstate_t state{};

  size_t rd;
  size_t* const lpread = (nullptr == readed) ? &rd : readed;
  *lpread = 0;

  auto& cvt = std::use_facet<XCODECVT>(locale);

  const auto need = u8.size();

  std::wstring ws;
  ws.resize(need, L'\0');

  const char* from        = (const char*)u8.data();
  const char* from_end    = from + u8.size();
  const char* from_next;
  
  wchar_t* to             = ws.data();
  wchar_t* to_end         = to + ws.size();
  wchar_t* to_next;

  const auto result = cvt.in(
    state,
    from,   from_end,   from_next,
    to,     to_end,     to_next);

  *lpread = from_next - from;
  ws.resize(to_next - to);

  return (XCODECVT::ok != result && nullptr == readed) ? std::wstring() : ws;
  }

/**
  UNICODE 串转换 UTF8 串。
  \param    ws    需要转换的 UNICODE 串。
  \return         转换后的对应 UTF8 串。

  \code
    auto u8(ws2u8(L"文字"));
  \endcode
  */
inline std::u8string ws2u8(const std::wstring& ws, size_t* const readed = nullptr)
  {
  if(ws.empty())  return std::u8string();

  std::locale locale(LOCALE_WS_U8);

  std::mbstate_t state{};

  size_t rd;
  size_t* lpread = (nullptr == readed) ? &rd : readed;
  *lpread = 0;

  auto& cvt = std::use_facet<XCODECVT>(locale);

  const auto need = ws.size() * cvt.max_length();
  
  std::u8string u8;
  u8.resize(need, '\0');

  const wchar_t* from       = ws.data();
  const wchar_t* from_end   = from + ws.size();
  const wchar_t* from_next;

  char* to                  = (char*)u8.data();
  char* to_end              = to + u8.size();
  char* to_next;

  const auto result = cvt.out(
    state,
    from,   from_end,   from_next,
    to,     to_end,     to_next);

  *lpread = from_next - from;
  u8.resize(to_next - to);

  return (XCODECVT::ok != result && nullptr == readed) ? std::u8string() : u8;
  }

/**
  ANSI 串转换 UTF8 串。
  \param    as    需要转换的 ANSI 串。
  \return         转换后的对应 UTF8 串。

  \code
    auto u8(as2u8("文字"));
  \endcode
  */
inline std::u8string as2u8(const std::string& as, size_t* const readed = nullptr)
  {
  return ws2u8(as2ws(as, readed));
  }

/**
  UTF8 串转换 ANSI 串。
  \param    u8    需要转换的 UTF8 串。
  \return         转换后的对应 ANSI 串。

  \code
    auto as(u82as(u8"文字"));
  \endcode
*/
inline std::string u82as(const std::u8string& u8, size_t* const readed = nullptr)
  {
  return ws2as(u82ws(u8, readed));
  }

#endif  // _XLIB_AS_WS_U8_H_