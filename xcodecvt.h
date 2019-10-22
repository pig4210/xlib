/**
  \file  xcodecvt.h
  \brief 用于 ANSI 与 UNICODE 及 UTF8 等编码的本地化转换。

  \version    1.0.0.190925
  \note       Only for Ring3

  \author     triones
  \date       2019-08-02

  \section more 额外说明

  - linux 下，注意查询本地化支持： `locale -a` 。
    - 安装 GB2312 ： `sudo locale-gen zh_CN` 。
    - 安装 UTF8 中文： `sudo apt-get install language-pack-zh-hans` 。
  - g++ 默认编码为 UTF8 ，如需以 ASCII 编译，需加入编译参数： `-fexec-charset=GB2312` 。

  \section history 版本记录

  - 2019-08-05 新建。 0.1 。
  - 2019-09-25 使用 codecvt 重构。 1.0 。
*/
#ifndef _XLIB_XCODECVT_H_
#define _XLIB_XCODECVT_H_

#include <string>
#include <locale>

class xcodecvt
  {
  public:
    xcodecvt() = delete;
    xcodecvt(const xcodecvt&) = delete;
    xcodecvt& operator=(const xcodecvt&) = delete;
  public:
    using T = std::codecvt<wchar_t, char, std::mbstate_t>;
  public:
    // 注意 mbstate_t 必须用列表初始化，不能使用默认初始化。否则会在转换时出错。
    explicit xcodecvt(const char* const loc):
      locale(loc), state{}, result(T::ok)
      {
      }
    void reset()
      {
      state = std::mbstate_t{};
      result = T::ok;
      }
    std::wstring mb2ws(const std::string& mb, size_t* readed = nullptr)
      {
      reset();

      size_t rd;
      size_t* lpread = (nullptr == readed) ? &rd : readed;
      *lpread = 0;

      if(mb.empty())  return std::wstring();
      auto& cvt = std::use_facet<T>(locale);

      const auto need = mb.size();

      std::wstring ws;
      ws.resize(need, L'\0');

      const char* from = mb.c_str();
      const char* from_end = from + mb.size();
      const char* from_next;
      
      wchar_t* to = (wchar_t*)ws.c_str();
      wchar_t* to_end = to + ws.size();
      wchar_t* to_next;

      result = cvt.in(state,
                      from, from_end, from_next,
                      to, to_end, to_next);

      *lpread = from_next - from;
      ws.resize(to_next - to);

      return (T::ok != result && nullptr == readed) ? std::wstring() : ws;
      }
    std::string ws2mb(const std::wstring& ws, size_t* readed = nullptr)
      {
      reset();

      size_t rd;
      size_t* lpread = (nullptr == readed) ? &rd : readed;
      *lpread = 0;

      if(ws.empty())  return std::string();
      auto& cvt = std::use_facet<T>(locale);

      const auto need = ws.size() * cvt.max_length();
      
      std::string mb;
      mb.resize(need, '\0');

      const wchar_t* from = ws.c_str();
      const wchar_t* from_end = from + ws.size();
      const wchar_t* from_next;

      char* to = (char*)mb.c_str();
      char* to_end = to + mb.size();
      char* to_next;

      result = cvt.out(state,
                       from, from_end, from_next,
                       to, to_end, to_next);

      *lpread = from_next - from;
      mb.resize(to_next - to);

      return (T::ok != result && nullptr == readed) ? std::string() : mb;
      }
  private:
    std::locale locale;
  public:
    std::mbstate_t state;               ///< 允许查询状态。
    std::codecvt_base::result result;   ///< 允许查询结果。
  };

/// 允许通过设置 XCODECVTDEF 宏，改变默认编码。
#ifndef XCODECVTDEF
#ifdef _WIN32
#define XCODECVTDEF   ("zh-CN")
#else
#define XCODECVTDEF   ("zh_CN.GB2312")
#endif
#endif

/// 允许通过设置 XCODECVTUTF8 宏，改变默认 UTF8 编码。
#ifndef XCODECVTUTF8
#ifdef _WIN32
#define XCODECVTUTF8  ("zh-CN.UTF8")
#else
#define XCODECVTUTF8  ("zh_CN.UTF8")
#endif
#endif

/**
  ANSI 串转换 UNICODE 串。
  \param  as    需要转换的 ANSI 串
  \return       转换后的对应 ANSI 串对象。

  \code
    std::wcout << as2ws("文字");
  \endcode
  */
inline std::wstring as2ws(const std::string& as, size_t* readed = nullptr)
  {
  return xcodecvt(XCODECVTDEF).mb2ws(as, readed);
  }

/**
  UNICODE 串转换 ANSI 串。
  \param  ws    需要转换的 UNICODE 串。
  \return       转换后的对应 ANSI 串对象。

  \code
    std::cout << ws2as(L"文字");
  \endcode
*/
inline std::string ws2as(const std::wstring& ws, size_t* readed = nullptr)
  {
  return xcodecvt(XCODECVTDEF).ws2mb(ws, readed);
  }

/**
  UTF8 串转换 UNICODE 串。
  \param    u8    需要转换的 UTF8 串。
  \return         转换后的对应 UNICODE 串对象。

  \code
    auto ws(u82ws(u8"文字"));
  \endcode
*/
inline std::wstring u82ws(const std::string& u8, size_t* readed = nullptr)
  {
  return xcodecvt(XCODECVTUTF8).mb2ws(u8, readed);
  }

/**
  UNICODE 串转换 UTF8 串。
  \param    ws    需要转换的 UNICODE 串。
  \return         转换后的对应 UTF8 串对象。

  \code
    auto u8(ws2u8(L"文字"));
  \endcode
  */
inline std::string ws2u8(const std::wstring& ws, size_t* readed = nullptr)
  {
  return xcodecvt(XCODECVTUTF8).ws2mb(ws, readed);
  }

/**
  ANSI 串转换 UTF8 串。
  \param    as    需要转换的 ANSI 串。
  \return         转换后的对应 UTF8 串对象。

  \code
    auto u8(as2u8("文字"));
  \endcode
  */
inline std::string as2u8(const std::string& as, size_t* readed = nullptr)
  {
  return ws2u8(as2ws(as, readed));
  }

/**
  UTF8 串转换 ANSI 串。
  \param    u8    需要转换的 UTF8 串。
  \return         转换后的对应 ANSI 串对象。

  \code
    auto as(u82as(u8"文字"));
  \endcode
*/
inline std::string u82as(const std::string& u8, size_t* readed = nullptr)
  {
  return ws2as(u82ws(u8, readed));
  }

#endif  // _XLIB_XCODECVT_H_