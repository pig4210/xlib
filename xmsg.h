/**
  \file  xmsg.h
  \brief 定义了信息组织的基本类，类似标准库的 ostreamstring 。

  \version    1.3.0.161115
  \note       For All

  \author     triones
  \date       2011-07-22

  \section history 版本记录

  - 2012-06-06 从 xlog 中分离，重新定义为 xmsg 。 0.1 。
  - 2012-06-06 考虑 xmsg 继承 mem_buffer ，需要新建不少函数，暂不施行。
  - 2012-07-19 优化 prt 。 0.1.1
  - 2012-10-09 新增对 int64 的支持。其它格式化作了些优化。 0.2 。
  - 2012-10-23 使 xmsg 信息组织后数据包含结尾 0 。 xmsg 初始时为 "\0" 而非空串。
  - 2012-10-23 重载 end 、 empty 。 0.3 。
  - 2013-03-05 重载 clear 以修复缓冲清空后的小 BUG 。 0.3.1 。
  - 2014-01-11 引入 SGISTL ，修改适应标准库。 1.0 。
  - 2014-04-09 修改一些数据的输出格式。 1.1 。
  - 2016-07-20 添加 xmsg 构造。 1.2 。
  - 2016-11-15 适配 Linux g++ 。处理不再附加结尾 0 。 1.3 。
*/
#ifndef _XLIB_XMSG_H_
#define _XLIB_XMSG_H_

#include <string>

#include "ws_utf8.h"

#ifdef FOR_RING0
#   pragma comment(lib,"libcntpr.lib")  // RING0 下浮点需连接此库，否则链接出错。
#endif

class xmsg : public std::string
  {
  public:
    xmsg();
    xmsg(const std::string& s);
    xmsg(const ucs2string& s);
  public:
    xmsg& prt(const char* const fmt, ...);      ///< 指定格式输出。
    xmsg& operator<<(const void* v);            ///< 输出 hex 指针。
    xmsg& operator<<(const bool& v);            ///< 输出 :true :false。
    xmsg& operator<<(const char& v);            ///< 输出 字符。
    xmsg& operator<<(const char* v);            ///< 输出 字符串。
    xmsg& operator<<(const charucs2_t& v);      ///< 输出 UNICCODE(UCS2) 转化后 ASCII。
    xmsg& operator<<(const charucs2_t* v);      ///< 输出 UNICCODE(UCS2) 字符串转化 ASCII 字符串。
    xmsg& operator<<(const float& v);           ///< 输出 dec 浮点数。
    xmsg& operator<<(const double& v);          ///< 输出 dec 浮点数。
    xmsg& operator<<(const std::string& v);     ///< 输出 内容。
    xmsg& operator<<(const ucs2string& v);      ///< 输出 UNICCODE(UCS2) 字符串转化 ASCII 字符串。
    xmsg& operator<<(const xutf8& v);           ///< 输出 UTF8 字符串转化字符串。
    xmsg& operator<<(xmsg& (*pfn)(xmsg&));      ///< 驱动函数。

    xmsg& operator<<(const uint8& v);           ///< 输出 hex(XX)。
    xmsg& operator<<(const int16& v);           ///< 输出 dec 值。
    xmsg& operator<<(const uint16& v);          ///< 输出 hex(XXXX) 。
    xmsg& operator<<(const int32& v);           ///< 输出 dec 值。
    xmsg& operator<<(const uint32& v);          ///< 输出 hex(XXXXXXXX) 。
    xmsg& operator<<(const int64& v);           ///< 输出 dec值。
    xmsg& operator<<(const uint64& v);          ///< 输出 hex(XXXXXXXXXXXXXXXX) 。
  };

#endif  // _XLIB_XMSG_H_