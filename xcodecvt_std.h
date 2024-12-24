/**
  \file  xcodecvt_std.h
  \brief 用于 ANSI 与 UNICODE 及 UTF8 等编码的本地化转换，使用 标准库。

  \version    2.4.1.241126

  \author     triones
  \date       2019-08-02

  \section more 额外说明

  - C++20 中，指出 codecvt 被弃用，虽然 VS2022 仍然支持，但这里独立成文件，以便替换。

  \section history 版本记录

  - 2019-08-05 新建。 0.1 。
  - 2019-09-25 使用 codecvt 重构。 1.0 。
  - 2019-11-03 引入 u8string ，再次重构。 2.0 。
  - 2020-05-14 加入贪婪处理方式。 2.1 。
  - 2021-08-04 处理 ws 转换 emoji 不正确的 BUG。 2.2 。
  - 2022-01-05 支持 c++17 。 2.3 。
  - 2022-08-25 支持替换为三方编码转换。
*/
#ifndef _XLIB_XCODECVT_STD_H_
#define _XLIB_XCODECVT_STD_H_

#include <locale>
#include <string>

namespace xlib {

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
#define LOCALE_WS_U8   ("zh-CN.UTF8")
#else
#define LOCALE_WS_U8   ("zh_CN.UTF8")
#endif
#endif

/**
  ANSI 串转换 UNICODE 串。
  \param  as      需要转换的 ANSI 串
  \param  lpread  返回转换 ANSI 字符数。指针默认为 nullptr 时，采用贪婪模式，转换失败的字符，以 ? 代替。
  \return         转换后的对应 UNICODE 串。

  \code
    auto ws = as2ws("文字");
  \endcode
*/
inline std::wstring as2ws(const std::string& as, size_t* const lpread = nullptr) {
  if (as.empty()) return {};

  size_t rd;
  size_t& read = (nullptr == lpread) ? rd : *lpread;
  read = 0;

  size_t write = 0;

  using XCODECVT = std::codecvt<wchar_t, char, std::mbstate_t>;
  std::locale locale(LOCALE_AS_WS);
  auto& cvt = std::use_facet<XCODECVT>(locale);

  std::wstring ws(as.size(), L'\0');

  while (read < as.size()) {
    std::mbstate_t state{};

    auto from       = as.data() + read;
    auto from_end   = as.data() + as.size();
    auto from_next  = from_end;

    auto to         = ws.data() + write;
    auto to_end     = ws.data() + ws.size();
    auto to_next    = to_end;

    const auto result = cvt.in(
        state,
        from,   from_end,   from_next,
        to,     to_end,     to_next);

    read  += from_next - from;
    write += to_next - to;

    if (nullptr != lpread) break;
    if (XCODECVT::ok != result) {
      ++read;
      *(ws.data() + write) = L'?';
      ++write;
    }
  }

  ws.resize(write);
  return ws;
}

/**
  UNICODE 串转换 ANSI 串。
  \param  ws      需要转换的 UNICODE 串。
  \param  lpread  返回转换 ANSI 字符数。指针默认为 nullptr 时，采用贪婪模式，转换失败的字符，以 ? 代替。
  \return         转换后的对应 ANSI 串。

  \code
    auto as = ws2as(L"文字");
  \endcode
*/
inline std::string ws2as(const std::wstring& ws, size_t* const lpread = nullptr) {
  if (ws.empty()) return {};

  size_t rd;
  size_t& read = (nullptr == lpread) ? rd : *lpread;
  read = 0;

  size_t write = 0;

  using XCODECVT = std::codecvt<wchar_t, char, std::mbstate_t>;
  std::locale locale(LOCALE_AS_WS);
  auto& cvt = std::use_facet<XCODECVT>(locale);

  std::string as(ws.size() * cvt.max_length(), '\0');

  while (read < ws.size()) {
    std::mbstate_t state{};

    auto from       = ws.data() + read;
    auto from_end   = ws.data() + ws.size();
    auto from_next  = from_end;

    auto to         = as.data() + write;
    auto to_end     = as.data() + as.size();
    auto to_next    = to_end;

    const auto result = cvt.out(
        state,
        from,   from_end,   from_next,
        to,     to_end,     to_next);

    read  += from_next - from;
    write += to_next - to;

    if (nullptr != lpread) break;
    if (XCODECVT::ok != result) {
      ++read;
      *(as.data() + write) = '?';
      ++write;
    }
  }

  as.resize(write);
  return as;
}

/**
  UTF8 串转换 UNICODE 串。
  \param  u8      需要转换的 UTF8 串。
  \param  lpread  返回转换 ANSI 字符数。指针默认为 nullptr 时，采用贪婪模式，转换失败的字符，以 ? 代替。
  \return         转换后的对应 UNICODE 串。

  \code
    auto ws(u82ws(u8"文字"));
  \endcode
*/
inline std::wstring u82ws(const std::u8string& u8, size_t* const lpread = nullptr) {
  if (u8.empty()) return {};

  size_t rd;
  size_t& read = (nullptr == lpread) ? rd : *lpread;
  read = 0;

  size_t write = 0;

#ifdef _WIN32
  using WCHART = char16_t;
#else
  using WCHART = char32_t;
#endif
#ifndef __cpp_lib_char8_t
  using U8CHART = char;
#else
  using U8CHART = char8_t;
#endif
#pragma warning(push)
#pragma warning(disable:4996)
  using XCODECVT = std::codecvt<WCHART, U8CHART, std::mbstate_t>;
  std::locale locale(LOCALE_WS_U8);
  auto& cvt = std::use_facet<XCODECVT>(locale);

  std::wstring ws(u8.size(), L'\0');

  while (read < u8.size()) {
    std::mbstate_t state{};

    auto from       = (const U8CHART*)u8.data() + read;
    auto from_end   = (const U8CHART*)u8.data() + u8.size();
    auto from_next  = from_end;

    auto to         = (WCHART*)ws.data() + write;
    auto to_end     = (WCHART*)ws.data() + ws.size();
    auto to_next    = to_end;

    const auto result = cvt.in(
        state,
        from,   from_end,   from_next,
        to,     to_end,     to_next);

    read  += from_next - from;
    write += to_next - to;

    if (nullptr != lpread) break;
    if (XCODECVT::ok != result) {
      ++read;
      *(ws.data() + write) = L'?';
      ++write;
    }
  }
#pragma warning(pop)
  ws.resize(write);
  return ws;
}

/**
  UNICODE 串转换 UTF8 串。
  \param  ws      需要转换的 UNICODE 串。
  \param  lpread  返回转换 ANSI 字符数。指针默认为 nullptr 时，采用贪婪模式，转换失败的字符，以 ? 代替。
  \return         转换后的对应 UTF8 串。

  \code
    auto u8(ws2u8(L"文字"));
  \endcode
  */
inline std::u8string ws2u8(const std::wstring& ws, size_t* const lpread = nullptr) {
  if (ws.empty()) return {};

  size_t rd;
  size_t& read = (nullptr == lpread) ? rd : *lpread;
  read = 0;

  size_t write = 0;

#ifdef _WIN32
  using WCHART = char16_t;
#else
  using WCHART = char32_t;
#endif
#ifndef __cpp_lib_char8_t
  using U8CHART = char;
#else
  using U8CHART = char8_t;
#endif
#pragma warning(push)
#pragma warning(disable:4996)
  using XCODECVT = std::codecvt<WCHART, U8CHART, std::mbstate_t>;
  std::locale locale(LOCALE_WS_U8);
  auto& cvt = std::use_facet<XCODECVT>(locale);

  std::u8string u8(ws.size() * cvt.max_length(), char8_t('\0'));

  while (read < ws.size()) {
    std::mbstate_t state{};

    auto from       = (const WCHART*)ws.data() + read;
    auto from_end   = (const WCHART*)ws.data() + ws.size();
    auto from_next  = from_end;

    auto to         = (U8CHART*)u8.data() + write;
    auto to_end     = (U8CHART*)u8.data() + u8.size();
    auto to_next    = to_end;

    const auto result = cvt.out(
        state,
        from,   from_end,   from_next,
        to,     to_end,     to_next);

    read  += from_next - from;
    write += to_next - to;

    if (nullptr != lpread) break;
    if (XCODECVT::ok != result) {
      ++read;
      *(u8.data() + write) = char8_t(u8'?');
      ++write;
    }
  }
#pragma warning(pop)
  u8.resize(write);
  return u8;
}

/**
  ANSI 串转换 UTF8 串。
  \param    as    需要转换的 ANSI 串。
  \return         转换后的对应 UTF8 串。

  \code
    auto u8(as2u8("文字"));
  \endcode
  */
inline std::u8string as2u8(const std::string& as, size_t* const lpread = nullptr) {
  for (const auto& c : as) {
    const uint8_t ch = (uint8_t)c;
    if (!(isprint(ch) || isspace(ch))) return ws2u8(as2ws(as, lpread));
  }
  // 纯英文字符，无需转换。
  size_t rd;
  size_t& read = (nullptr == lpread) ? rd : *lpread;
  read = as.size();
  return std::u8string((const char8_t*)as.data(), as.size());
}

/**
  UTF8 串转换 ANSI 串。
  \param    u8    需要转换的 UTF8 串。
  \return         转换后的对应 ANSI 串。

  \code
    auto as(u82as(u8"文字"));
  \endcode
*/
inline std::string u82as(const std::u8string& u8, size_t* const lpread = nullptr) {
  for (const auto& c : u8) {
    const uint8_t ch = (uint8_t)c;
    if (!(isprint(ch) || isspace(ch))) return ws2as(u82ws(u8, lpread));
  }
  // 纯英文字符，无需转换。
  size_t rd;
  size_t& read = (nullptr == lpread) ? rd : *lpread;
  read = u8.size();
  return std::string((const char*)u8.data(), u8.size());
}

#undef LOCALE_AS_WS
#undef LOCALE_WS_U8

}  // namespace xlib

#endif  // _XLIB_XCODECVT_STD_H_