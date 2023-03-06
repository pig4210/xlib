/**
  \file  xcodecvt_win.h
  \brief 用于 ANSI 与 UNICODE 及 UTF8 等编码的本地化转换，使用 WindowsAPI 。

  \version    0.0.1.230224

  \author     triones
  \date       2022-08-24

  \section history 版本记录

  - 2022-08-24 新建 。 0.0.1 。
*/
#ifndef _XLIB_XCODECVT_WIN_H_
#define _XLIB_XCODECVT_WIN_H_

#include <string>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef NOMINMAX
#undef WIN32_LEAN_AND_MEAN

#ifndef _WIN32
#error "xcodecvt_win.h only for windows !"
#endif

namespace xlib {

/// 允许通过设置 LOCALE_AS_WS 宏，改变默认 ANSI 编码。
#ifndef LOCALE_AS_WS
#define LOCALE_AS_WS  CP_ACP
#endif

/// 允许通过设置 LOCALE_WS_U8 宏，改变默认 UTF8 编码。
#ifndef LOCALE_WS_U8
#define LOCALE_WS_U8  CP_UTF8
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
  if (as.empty()) return std::wstring();

  size_t rd;
  size_t& read = (nullptr == lpread) ? rd : *lpread;
  read = 0;

  const auto need = as.size();

  std::wstring ws;
  ws.resize(need, L'\0');

  const DWORD dwFlags = (nullptr == lpread) ? 0 : MB_ERR_INVALID_CHARS;

  for (rd = as.size(); rd != 0; --rd) {
#pragma warning(push)
#pragma warning(disable : 4267)
    const auto write = MultiByteToWideChar(LOCALE_AS_WS, dwFlags,
                                           as.data(), rd,
                                           ws.data(), ws.size());
#pragma warning(pop)
    if (write > 0) {
      read = rd;
      ws.resize(write);
      return ws;
    }
  }
  read = 0;
  return std::wstring();
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
  if (ws.empty()) return std::string();

  size_t rd;
  size_t& read = (nullptr == lpread) ? rd : *lpread;
  read = 0;

  const auto need = ws.size() * 2;

  std::string as;
  as.resize(need, '\0');

  for (rd = ws.size(); rd != 0; --rd) {
    BOOL UsedDefultChar = FALSE;
#pragma warning(push)
#pragma warning(disable : 4267)
    const auto write = WideCharToMultiByte(LOCALE_AS_WS, 0,
                                           ws.data(), rd,
                                           as.data(), as.size(),
                                           nullptr, &UsedDefultChar);
#pragma warning(pop)
    if (write > 0) {
      if (nullptr != lpread && UsedDefultChar == TRUE) continue;
      read = rd;
      as.resize(write);
      return as;
    }
  }

  read = 0;
  return std::string();
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
  if (u8.empty()) return std::wstring();

  size_t rd;
  size_t& read = (nullptr == lpread) ? rd : *lpread;
  read = 0;

  const auto need = u8.size();

  std::wstring ws;
  ws.resize(need, L'\0');

  const DWORD dwFlags = (nullptr == lpread) ? 0 : MB_ERR_INVALID_CHARS;

  for (rd = u8.size(); rd != 0; --rd) {
#pragma warning(push)
#pragma warning(disable : 4267)
    const auto write = MultiByteToWideChar(LOCALE_WS_U8, dwFlags,
                                           (const char*)u8.data(), rd,
                                           ws.data(), ws.size());
#pragma warning(pop)
    if (write > 0) {
      read = rd;
      ws.resize(write);
      return ws;
    }
  }
  read = 0;
  return std::wstring();
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
inline std::u8string ws2u8(const std::wstring& ws,
                           size_t* const lpread = nullptr) {
  if (ws.empty()) return std::u8string();

  size_t rd;
  size_t& read = (nullptr == lpread) ? rd : *lpread;
  read = 0;

  const auto need = ws.size() * 4;

  std::u8string u8;
  u8.resize(need, '\0');

  for (rd = ws.size(); rd != 0; --rd) {
    BOOL UsedDefultChar = FALSE;
#pragma warning(push)
#pragma warning(disable : 4267)
    const auto write = WideCharToMultiByte(LOCALE_WS_U8, 0,
                                           ws.data(), rd,
                                           (char*)u8.data(), u8.size(),
                                           nullptr, &UsedDefultChar);
#pragma warning(pop)
    if (write > 0) {
      if (nullptr != lpread && UsedDefultChar == TRUE) continue;
      read = rd;
      u8.resize(write);
      return u8;
    }
  }

  read = 0;
  return std::u8string();
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
  return std::u8string((const char8_t*)as.c_str(), as.size());
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
  return std::string((const char*)u8.c_str(), u8.size());
}

#undef LOCALE_AS_WS
#undef LOCALE_WS_U8

}  // namespace xlib

#endif  // _XLIB_XCODECVT_WIN_H_