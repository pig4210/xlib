/**
  \file  ws_utf8.h
  \brief 用于 UTF8 与 UNICODE(UCS2) 相互转换。

  \version    1.3.0.170103
  \note       For All

  \author     triones
  \date       2013-03-07

  \details    转换使用自己实现的过程，不采用网上通用转换 或 高版本 Windows API，为记。
  
  \section history 版本记录

  - 2013-03-07 新增 UNICODE(UCS2) 与 UTF8 编码的转换。 0.1 。
  - 2013-03-08 发现 WinXP 的 ntdll 不提供 UTF 函数，故参考 UTF8 文档，重新实现。 0.2 。
  - 2013-03-09 扩展支持 4 byte Unicode 。 0.3 。
  - 2013-05-25 处理转换时多处理一个数据的 BUG。 1.0 。
  - 2014-01-11 引入 SGISTL，修改适应标准库。 1.1 。
  - 2014-04-08 修正 utf8_byte2unicode_byte 的一个 BUG ，修正一些算法细节。 1.1.1 。
  - 2014-04-17 细节改善。 1.1.2 。
  - 2014-04-18 转变 utf8_byte2unicode_byte 使不自动跳过非法、不完整字符。 1.1.3 。
  - 2014-05-07 细节改进。 1.2 。
  - 2017-01-03 适配 Linux g++ 。处理不再附加结尾 0 。简化接口。 1.3 。
*/
#ifndef _XLIB_WS_UTF8_H_
#define _XLIB_WS_UTF8_H_

#include <string>

#include "xlib_base.h"
#include "ws_s.h"

typedef uint8*                    p_utf8;
typedef std::basic_string<uint8>  xutf8;
typedef const uint8*              pc_utf8;

/**
  转换一个 UNICODE(UCS2) 字符为一个 UTF8 字符。
  \param    ucs2      UNICODE(UCS2) 字符。
  \return             返回转换结果。
*/
xutf8 unicode_byte2utf8_byte(const uint32 unicode);

/**
  转换一个 UTF8 字符为一个 UNICODE(UCS2) 字符，非法 UTF8 字符或不完整 UTF8 字符则无法转换。
  \param    unicode   结果缓冲，可为 nullptr 。
  \param    utf8      UTF8 字符指针。
  \return             返回读取 UTF8 字节数，返回 0 表示失败。
*/
size_t utf8_byte2unicode_byte(uint32* unicode, pc_utf8 const utf8);

/**
  UNICODE(UCS2) 串转换 UTF8 串。
  \param    ws    需要转换的 UNICODE(UCS2) 串。
  \param    size  需要转换的 UNICODE(UCS2) 串长度（以宽字计）。
  \return         转换后的对应 UTF8 串对象。

  \code
    auto s(ws2utf8(L"文字"));
    if(s.empty())
      {
      cout << "ws2utf8 转换出错，LastError：" << GetLastError();
      }
  \endcode
  */
xutf8 ws2utf8(const charucs2_t* const ws, const size_t size);
xutf8 ws2utf8(const ucs2string& ws);

/**
  UTF8 串转换 UNICODE(UCS2) 串。
  \param    utf8  需要转换的 UTF8 串。
  \param    size  需要转换的 UTF8 串长度（以 byte 计）。
  \return         转换后的对应 UNICODE(UCS2) 串对象。
*/
ucs2string utf82ws(pc_utf8 const utf8, const size_t size);
ucs2string utf82ws(const xutf8& utf8);

#endif  // _XLIB_WS_UTF8_H_