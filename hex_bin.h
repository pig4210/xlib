/**
  \file  hex_bin.h
  \brief 定义了 hex 与 bin 的转换操作。

  \version    1.7.2.190620
  \note       For All

  \author     triones
  \date       2010-03-03
  
  \section history 版本记录

  - 2010-12-09 添加结构体 hex_character ，并对 hex2str 与 str2hex 函数进行相应改造，去除强制移位计算。
  - 2010-12-29 hex&str 独立剥离，新建文件。 0.1 。
  - 2011-04-20 hex2str 新加默认参数，提升 str2hex 功能。 0.2 。
  - 2011-06-02 新加重载的 hex2str 、 str2hex 函数，引入 line 类的返回。 0.3 。
  - 2011-08-29 新加一组读取各种进制 ASCII 字符的转换函数，新加单个 HEX 值转换 ASCII 函数。 0.4 。
  - 2011-08-29 引入 hexvalue 模板。 0.5 。
  - 2011-12-20 撤销之前添加的一组转换函数，同时撤销 hexvalue 模板。另设计一个函数以替换之。 0.6 。
  - 2012-02-27 发现 str2hex 的一个 BUG ，已修正。 0.6.1 。
  - 2012-06-07 删除 Hex2Ascii 。 0.7 。
  - 2013-03-05 修改返回 xstr 为返回 xmsg 。 0.8 。
  - 2013-03-20 新增 str2hexs 函数。 0.9 。
  - 2013-10-24 改进 hex2show 对 ASCII 中文输出的优化。 0.10 。
  - 2013-11-13 改进 hex2show 可以输出 UTF8 。 0.11 。
  - 2013-12-10 改进 str2hex 以适应 x64 。 0.12 。
  - 2014-01-13 引入 SGISTL ，做适应性修改。 1.0 。
  - 2014-02-19 修正 str2hex 的一个小 BUG 。 1.0.1 。
  - 2014-04-09 修正 str2hex 的一个小 BUG 。 1.0.2 。
  - 2014-04-18 改进 hex2show 的字符集显示算法。 1.1 。
  - 2014-05-08 细节改进。 1.1.1 。
  - 2014-07-24 新加 hex2show 模版。 1.2 。
  - 2014-08-13 修正 str2hexs 的一个严重 BUG 。 1.3 。
  - 2016-11-15 适配 Linux g++ 。 1.4 。
  - 2017-01-03 简化接口。 1.5 。
  - 2017-07-14 修改名称以符合通用称呼。 1.6 。
  - 2018-02-07 改进 showbin 的内核调用方式，使得参数进出明确。 1.7 。
  - 2018-02-13 修正 hex2bin 的一个返回错误。 1.7.1 。
  - 2019-06-20 放弃 escape 模板。 1.7.2 。
*/
#ifndef _XLIB_HEX_BIN_H_
#define _XLIB_HEX_BIN_H_

#include <string>

#include "xlib_base.h"

/**
  十六进制值的结构体。主要体现 BYTE 与十六进制字符间的关系。

  \code
    BIN_VALUE_STRUCT bins = {1, 4};  // bins 为字符 'A' 。
  \endcode
*/
#pragma pack (push, 1)
struct BIN_VALUE_STRUCT
  {
  uint8 low:  4;
  uint8 high: 4;
  };
#pragma pack (pop)

/// 十六进制 ASCII 表现形式的结构体。
struct HEX_VALUE_STRUCT
  {
  int8  high;
  int8  low;
  };

/**
  指定 BIN 串转换为 HEX 格式。
  \param  bin   源 BIN 串。
  \param  size  源 BIN 串大小。
  \param  isup  指定转换后的 ASCII 大小写，默认小写。
  \return       返回转换后的 ASCII 串对象。

  \code
    string asc = bin2hex(string("\xAB\xCD\xEF"));
    cout << "bin2hex:" << asc ;   // 将输出 "abcdef" 。
  \endcode
*/
std::string bin2hex(const void* bin, const size_t size, bool isup = false);

/**
  指定 HEX 串转换为值。
  \param  hex       源 HEX 串。
  \param  size      源 HEX 串大小。
  \param  lpreadlen 成功读取 BIN 串时，返回读取的字符个数。\n
                    lpreadlen 可以为 nullptr ，此时不返回结果。\n
                    转换失败， *lpreadlen == 0 。
  \param  wantlen   需要处理的 \b 有效数据 大小 （ 0 <= wantlen <= 8 ）。\n
                    当 <= 0 或 > 8 时，一律视 == 8 。 x64 下大小为 16 。
  \param  errexit   指定转换未结束前遭遇非 HEX 字符，是否视作失败处理。\n
                    默认不作失败处理。当此标志为 true 时，忽略 errbreak 。
  \param  errbreak  指定转换未结束前遭遇非 HEX 字符，是否中止处理。默认跳过。
  \return           成功转换则为十六进制 ASCII 串对应的 HEX 值。\n
                    判定转换失败，应该通过 lpreadlen 的返回结果判定。

  \code
    unsigned int readlen = 0;
    const unsigned int value = hex2value("12345678", readlen);
    // 返回 value == 0x12345678; readlen == 8;
    const unsigned int value = hex2value("1234 56|78", readlen);
    // 返回 value == 0x12345678; readlen == 10;
    const unsigned int value = hex2value("1234 56|78", readlen, 8, true);
    // 返回 value == 0; readlen == 0;
    const unsigned int value = hex2value("1234 56|78", readlen, 8, false, ture);
    // 返回 value == 0x1234; readlen == 4;
  \endcode
*/
size_t hex2value(const void*    hex,
                 const size_t   size,
                 size_t*        lpreadlen = nullptr,
                 size_t         wantlen   = 0,
                 const bool     errexit   = false,
                 const bool     errbreak  = false);

/**
  指定 HEX 串转换为 BIN 串。\n
  转换 ASCII 的十六进制字符应成对的，如最后字符不成对，将忽略最后不成对的字符。
  \param  hex       源 HEX 串。
  \param  size      源串大小。
  \param  lpreadlen 成功读取 HEX 串时，返回读取的字符个数。\n
                    lpreadlen 可以为 nullptr ，此时不返回结果。\n
                    转换失败， *lpreadlen == 0。
  \param  errexit   指定转换未结束前遭遇非 HEX 字符，是否视作失败处理。\n
                    默认不作失败处理。当此标志为 true 时，忽略 errbreak 。
  \param  errbreak  指定转换未结束前遭遇非 HEX 字符，是否中止处理。默认跳过。
  \return           成功转换则为十六进制 ASCII 串对应的 HEX 值。\n
                    判定转换失败，应该通过 lpreadlen 的返回结果判定。
*/
std::string hex2bin(const void*   hex,
                    const size_t  size,
                    size_t*       lpreadlen  = nullptr,
                    const bool    errexit    = false,
                    const bool    errbreak   = false);

/**
  指定ASCII串，分析转义字符。

  可转义的字符有：
  \code
    \0、\a、\b、\f、\n、\r、\t、\v、\\、\'、\"、\?
  \endcode
  另可转义 \x######## 。
  \x 识别十六进制数据时，根据读取长度，自动匹配类型 （ 2:byte, 4:short, 8:int ）。
  \param    strs    源 ASCII 串（转换字串最大长度为 0xFFFF ）。
  \param    strs    源 ASCII 串大小。
  \return           返回转换后的 ASCII 串对象。
*/
std::string escape(const void* strs, const size_t size);

enum Hex2showCode
  {
  HC_ASCII,
  HC_UNICODE,
  HC_UTF8,
  };

/**
  指定 HEX 串，格式化显示。
  \param  data      HEX 串。
  \param  code      指明内容编码（默认 HC_ASCII ）。
  \param  isup      HEX 格式大小写控制。
  \param  prews     前缀空格数。
  \return           格式化后的内容。

  \code
    string ss = "123456789aasdfsdhcf";
    cout << showbin(ss) << endl;
    // 0012FF54┃31 32 33 34|35 36 37 38|39 61 61 73|64 66 73 64┃123456789aasdfsd
    // 0012FF64┃68 63 66 00|           |           |           ┃hcf.
  \endcode
*/
std::string showbin(const void*        data,
                    const size_t       size,
                    const Hex2showCode code  = HC_ASCII,
                    const bool         isup  = true,
                    const size_t       prews = 0);

//////////////////////////////////////////////////////////////////////////
template<typename T>
std::string bin2hex(const std::basic_string<T>& hexs,
                    const bool                  isup = false)
  {
  return bin2hex((void*)hexs.c_str(), hexs.size() * sizeof(T), isup);
  }

template<typename T>
size_t hex2value(const std::basic_string<T>&  hex,
                 size_t*                      lpreadlen = nullptr,
                 size_t                       wantlen = 0,
                 const bool                   errexit = false,
                 const bool                   errbreak = false)
  {
  return hex2value((void*)hex.c_str(),
                   hex.size() * sizeof(T),
                   lpreadlen,
                   wantlen,
                   errexit,
                   errbreak);
  }

template<typename T>
std::string hex2bin(const std::basic_string<T>&   hex,
                    size_t*                       lpreadlen = nullptr,
                    const bool                    errexit = false,
                    const bool                    errbreak = false)
  {
  return hex2bin((void*)hex.c_str(),
                 hex.size() * sizeof(T),
                 lpreadlen,
                 errexit,
                 errbreak);
  }

std::string escape(const std::string& strs);

template<typename T>
std::string showbin(const std::basic_string<T>&  data,
                    const Hex2showCode           code,
                    const bool                   isup,
                    const size_t                 prews)
  {
  return showbin((void*)data.c_str(), data.size() * sizeof(T),
                 code, isup, prews);
  }

template<typename T>
std::string showbin(const std::basic_string<T>& data)
  {
  return showbin((void*)data.c_str(), data.size() * sizeof(T),
                 HC_ASCII, true, 0);
  }

template<typename T>
std::string showbin(const std::basic_string<T>& data, const Hex2showCode code)
  {
  return showbin((void*)data.c_str(), data.size() * sizeof(T),
                  code, true, 0);
  }

template<typename T>
std::string showbin(const std::basic_string<T>& data, const bool isup)
  {
  return showbin((void*)data.c_str(), data.size() * sizeof(T),
                 HC_ASCII, isup, 0);
  }

template<typename T>
std::string showbin(const std::basic_string<T>& data, const size_t prews) // 需要强制类型哦。
  {
  return showbin((void*)data.c_str(), data.size() * sizeof(T),
                 HC_ASCII, true, prews);
  }

#endif  // _XLIB_HEX_BIN_H_