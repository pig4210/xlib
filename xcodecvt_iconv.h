/**
  \file  xcodecvt_iconv.h
  \brief 用于 ANSI 与 UNICODE 及 UTF8 等编码的本地化转换。使用 gnu libiconv 库。

  \version    0.0.1.241126

  \author     triones
  \date       2024-11-26

  \section history 版本记录

  - 2024-11-26 新建 。 0.0.1 。
*/
#ifndef _XLIB_XCODECVT_ICONV_H_
#define _XLIB_XCODECVT_ICONV_H_

#ifndef XLIB_XCODECVT_INCLUDE_GATE
#error "Please include xcodecvt.h instead"
#endif

#include <string>

#include <iconv.h>

namespace xlib {

/// 允许通过设置 LOCALE_AS_WS 宏，改变默认 ANSI 编码。
#ifndef LOCALE_AS_WS
#define LOCALE_AS_WS   ("GB2312")
#endif

/// 允许通过设置 LOCALE_WS_U8 宏，改变默认 UTF8 编码。
#ifndef LOCALE_WS_U8
#define LOCALE_WS_U8   ("UTF-8")
#endif

#ifdef _WIN32
using iconv_from_type = const char**;
#else
using iconv_from_type = char**;
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
  size_t& read = (lpread == nullptr) ? rd : *lpread;
  read = 0;

  size_t write = 0;

#ifdef _WIN32
  auto cvt = iconv_open("UTF-16LE", LOCALE_AS_WS);
#else
  auto cvt = iconv_open("WCHAR_T", LOCALE_AS_WS);
#endif
  if (cvt == (iconv_t)-1) return {};

  std::wstring ws(as.size(), L'\0');

  while (read < as.size()) {
    auto        from      = as.data() + read;
    size_t      from_left = as.size() - read;
    const auto  from_next = from_left;

    auto        to        = ws.data() + write;
    size_t      to_left   = (ws.size() - write) * sizeof(wchar_t);
    const auto  to_next   = to_left;

    const auto result = iconv(cvt, (iconv_from_type)&from, &from_left, (char**)&to, &to_left);

    read  += from_next - from_left;
    write += (to_next - to_left) / sizeof(wchar_t);

    if (nullptr != lpread) break;
    if (result == (size_t)-1) {
      ++read;
      *(ws.data() + write) = L'?';
      ++write;
    }
  }

  iconv_close(cvt);

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

#ifdef _WIN32
  auto cvt = iconv_open(LOCALE_AS_WS, "UTF-16LE");
#else
  auto cvt = iconv_open(LOCALE_AS_WS, "WCHAR_T");
#endif
  if (cvt == (iconv_t)-1) return {};

  std::string as(ws.size() * 6, '\0');

  while (read < ws.size()) {
    auto           from       = ws.data() + read;
    size_t         from_left  = (ws.size() - read) * sizeof(wchar_t);
    const auto     from_next  = from_left;

    auto           to         = as.data() + write;
    size_t         to_left    = as.size() - write;
    const auto     to_next    = to_left;

    const auto result = iconv(cvt, (iconv_from_type)&from, &from_left, &to, &to_left);

    read  += (from_next - from_left) / sizeof(wchar_t);
    write += to_next - to_left;
    
    if (nullptr != lpread) break;
    if (result == (size_t)-1) {
      ++read;
      *(as.data() + write) = '?';
      ++write;
    }
  }

  iconv_close(cvt);

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
  auto cvt = iconv_open("UTF-16LE", LOCALE_WS_U8);
#else
  auto cvt = iconv_open("WCHAR_T", LOCALE_WS_U8);
#endif
  if (cvt == (iconv_t)-1) return {};

  std::wstring ws(u8.size(), L'\0');

  while (read < u8.size()) {
    auto        from       = u8.data() + read;
    size_t      from_left  = u8.size() - read;
    const auto  from_next  = from_left;

    auto        to         = ws.data() + write;
    size_t      to_left    = (ws.size() - write) * sizeof(wchar_t);
    const auto  to_next    = to_left;

    const auto result = iconv(cvt, (iconv_from_type)&from, &from_left, (char**)&to, &to_left);

    read  += from_next - from_left;
    write += (to_next - to_left) / sizeof(wchar_t);

    if (nullptr != lpread) break;
    if (result == (size_t)-1) {
      ++read;
      *(ws.data() + write) = L'?';
      ++write;
    }
  }

  iconv_close(cvt);

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
  auto cvt = iconv_open(LOCALE_WS_U8, "UTF-16LE");
#else
  auto cvt = iconv_open(LOCALE_WS_U8, "WCHAR_T");
#endif
  if (cvt == (iconv_t)-1) return {};

  std::u8string u8(ws.size() * 6, char8_t('\0'));

  while (read < ws.size()) {
    auto           from       = ws.data() + read;
    size_t         from_left  = (ws.size() - read) * sizeof(wchar_t);
    const auto     from_next  = from_left;

    auto           to         = u8.data() + write;
    size_t         to_left    = u8.size() - write;
    const auto     to_next    = to_left;
    
    const auto result = iconv(cvt, (iconv_from_type)&from, &from_left, (char**)&to, &to_left);
    
    read  += (from_next - from_left) / sizeof(wchar_t);
    write += to_next - to_left;

    if (nullptr != lpread) break;
    if (result == (size_t)-1) {
      ++read;
      *(u8.data() + write) = char8_t(u8'?');
      ++write;
    }
  }

  iconv_close(cvt);

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
    if (!is_easy_transcoding(c)) return ws2u8(as2ws(as, lpread));
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
    if (!is_easy_transcoding(c)) return ws2as(u82ws(u8, lpread));
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

#endif  // _XLIB_XCODECVT_ICONV_H_