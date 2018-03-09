/*!
  \file  xmsg.h
  \brief xmsg.h定义了信息组织的基本类，类似标准库的ostreamstring

  \version    4.0.1611.1511
  \note       For All

  \author     triones
  \date       2011-07-22
*/
#ifndef _XLIB_XMSG_H_
#define _XLIB_XMSG_H_

#include <string>

#include "ws_utf8.h"

#ifdef FOR_RING0
#   pragma comment(lib,"libcntpr.lib")  // RING0下浮点需连接此库，否则链接出错
#endif

class xmsg : public std::string
  {
  public:
    xmsg();
    xmsg(const std::string& s);
    xmsg(const ucs2string& s);
  public:
    xmsg& prt(const char* const fmt, ...);      //!< 指定格式输出
    xmsg& operator<<(const void* v);            //!< 输出 hex指针
    xmsg& operator<<(const bool& v);            //!< 输出 :true :false
    xmsg& operator<<(const char& v);            //!< 输出 字符
    xmsg& operator<<(const char* v);            //!< 输出 字符串
    xmsg& operator<<(const charucs2_t& v);      //!< 输出 宽字转化后字符
    xmsg& operator<<(const charucs2_t* v);      //!< 输出 宽字符串转化字符串
    xmsg& operator<<(const float& v);           //!< 输出 dec浮点数
    xmsg& operator<<(const double& v);          //!< 输出 dec浮点数
    xmsg& operator<<(const std::string& v);     //!< 输出 内容
    xmsg& operator<<(const ucs2string& v);      //!< 输出 宽字符串转化字符串
    xmsg& operator<<(const xutf8& v);           //!< 输出 UTF8字符串转化字符串
    xmsg& operator<<(xmsg& (*pfn)(xmsg&));      //!< 驱动函数

    xmsg& operator<<(const uint8& v);           //!< 输出 hex(XX)
    xmsg& operator<<(const int16& v);           //!< 输出 dec值
    xmsg& operator<<(const uint16& v);          //!< 输出 hex(XXXX)
    xmsg& operator<<(const int32& v);           //!< 输出 dec值
    xmsg& operator<<(const uint32& v);          //!< 输出 hex(XXXXXXXX)
    xmsg& operator<<(const int64& v);           //!< 输出 dec值
    xmsg& operator<<(const uint64& v);          //!< 输出 hex(XXXXXXXXXXXXXXXX)
  };

#endif  // _XLIB_XMSG_H_