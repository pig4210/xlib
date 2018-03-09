/*!
  \file  ws_s.h
  \brief ws_s.h用于如ASCII与UNICODE(UCS2)相互转换

  - 使用内核函数转换，而不是使用转换API
  - Windows使用系统默认的ASCII编码（一般是GB2312）
  - Linux需要设置，默认GB2312。（使用ICONV库）
  - 由于Windows与Linux的Unicode默认不同，所以这里指的Unicode统一为UCS2
  - Linux下即使编译选项-fshort-wchar也不能用wchar_t和wstring，会产生莫名问题，
  
  \version    6.0.1612.2310
  \note       For All

  \author     triones
  \date       2011-11-11
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

//! UNICODE串转换ASCII串
/*!
  \param  ws    需要转换的UNICODE串
  \param  size  需要转换的UNICODE串的长度(以宽字计)
  \return       转换后的对应ASCII串对象

  \code
    #include "ws_s.h"
    string s(ws2s(L"文字"));
    if(s.empty())
      {
      cout << "ws2s转换出错，LastError：" << GetLastError();
      }
    else
      {
      cout << "转换结果：" << s;
      }
  \endcode
*/
std::string ws2s(const charucs2_t* const ws, const size_t size);
std::string ws2s(const ucs2string& ws);

//! ASCII串转换UNICODE串
/*!
  \param  s     需要转换的ASCII串
  \param  size  需要转换的ASCII串的长度
  \return       转换后的对应UNICODE串对象

  \code
    #include "ws_s.h"
    ucs2string ws(s2ws("文字"));
    if(ws.empty())
      {
      wcout << L"s2ws转换出错，LastError：" << GetLastError();
      }
    else
      {
      wcout << L"转换结果：" << ws;
      }
  \endcode
  */
ucs2string s2ws(const char* const s, const size_t size);
ucs2string s2ws(const std::string& s);

#ifndef _WIN32
//! 设置ASCII默认编码
/*!
  \param  encode  需要改变的默认编码\n
                  当encode == nullptr时，返回当前默认编码\n
                  当encode == ""时，设置默认编码为GB2312
  \return         设置后的编码
*/
const char* set_ascii_encode(const char* encode = nullptr);

#endif

#endif  // _XLIB_WS_S_H_