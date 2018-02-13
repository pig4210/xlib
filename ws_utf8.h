/*!
  \file  ws_utf8.h
  \brief ws_utf8.h用于如UTF8与UNICODE(UCS2)相互转换

  - 转换使用自己实现的过程。不采用网上通用转换，为记

  \version    4.0.1701.0314
  \note       For All

  \author     triones
  \date       2013-03-07
*/
#ifndef _XLIB_WS_UTF8_H_
#define _XLIB_WS_UTF8_H_

#include <string>

#include "xlib_base.h"
#include "ws_s.h"

typedef uint8*                    p_utf8;
typedef std::basic_string<uint8>  xutf8;
typedef const uint8*              pc_utf8;

/*!
  转换一个unicode字符为一个utf8字符
  \param    unicode   unicode字符
  \return             返回转换结果
*/
xutf8 unicode_byte2utf8_byte(const uint32 unicode);

/*!
  转换一个utf8字符为一个unicode字符，非法utf8字符或不完整utf8字符则无法转换
  \param    unicode   结果缓冲，可为nullptr
  \param    utf8      utf8字符指针
  \return             返回读取utf8字节数，返回0表示失败
*/
size_t utf8_byte2unicode_byte(uint32*       unicode,
                              pc_utf8 const utf8);

//! UNICODE串转换UTF8串
/*!
  \param    ws    需要转换的UNICODE串
  \param    size  需要转换的UNICODE串长度（以宽字计）
  \return         转换后的对应UTF8串对象

  \code
    auto s(ws2utf8(L"文字"));
    if(s.empty())
      {
      cout << "ws2utf8转换出错，LastError：" << GetLastError();
      }
  \endcode
  */
xutf8 ws2utf8(const charucs2_t* const ws, const size_t size);
xutf8 ws2utf8(const ucs2string& ws);

//! UTF8串转换UNICODE串
/*!
  \param    utf8  需要转换的UTF8串
  \param    size  需要转换的UTF8串
  \return         转换后的对应UNICODE串对象
*/
ucs2string utf82ws(pc_utf8 const utf8, const size_t size);
ucs2string utf82ws(const xutf8& utf8);

#endif  // _XLIB_WS_UTF8_H_