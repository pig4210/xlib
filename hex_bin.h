/*!
  \file  hex_bin.h
  \brief hex_bin.h定义了hex与bin的转换操作

  \version    9.2.1802.1313
  \note       For All

  \author     triones
  \date       2010-03-03
*/
#ifndef _XLIB_HEX_BIN_H_
#define _XLIB_HEX_BIN_H_

#include <string>

#include "xlib_base.h"

//! 十六进制值的结构体
/*!
  主要体现BYTE与十六进制字符间的关系

  \code
    BIN_VALUE_STRUCT bins = {1, 4};  //bins为字符'A'
  \endcode
*/
#pragma pack (push, 1)
struct BIN_VALUE_STRUCT
  {
  uint8 low:  4;
  uint8 high: 4;
  };
#pragma pack (pop)

//! 十六进制ASCII表现形式的结构体
struct HEX_VALUE_STRUCT
  {
  int8  high;
  int8  low;
  };

//! 指定BIN串转换为HEX格式
/*!
  \param  bin   源BIN串
  \param  size  源BIN串大小
  \param  isup  指定转换后的ASCII大小写，默认小写
  \return       返回转换后的ascii串对象

  \code
    string asc = bin2hex(string("\xAB\xCD\xEF"));
    cout << "bin2hex:" << asc ;   //将输出"abcdef"
  \endcode
*/
std::string bin2hex(const void* bin, const size_t size, bool isup = false);

//! 指定HEX串转换为值
/*!
  \param  hex       源HEX串
  \param  size      源HEX串大小
  \param  lpreadlen 成功读取BIN串时，返回读取的字符个数。\n
                    lpreadlen可以为nullptr，此时不返回结果。\n
                    转换失败，*lpreadlen == 0。
  \param  wantlen   需要处理的 \b 有效数据 大小(0<=wantlen<=8)。\n
                    当<=0或>8时，一律视==8。x64下大小为16
  \param  errexit   指定转换未结束前遭遇非HEX字符，是否视作失败处理。\n
                    默认不作失败处理。当此标志为true时，忽略errbreak。
  \param  errbreak  指定转换未结束前遭遇非HEX字符，是否中止处理。默认跳过
  \return           成功转换则为十六进制ASCII串对应的HEX值。\n
                    判定转换失败，应该通过lpreadlen的返回结果判定。

  \code
    unsigned int readlen = 0;
    const unsigned int value = hex2value("12345678", readlen);
    //返回value == 0x12345678; readlen == 8;
    const unsigned int value = hex2value("1234 56|78", readlen);
    //返回value == 0x12345678; readlen == 10;
    const unsigned int value = hex2value("1234 56|78", readlen, 8, true);
    //返回value == 0; readlen == 0;
    const unsigned int value = hex2value("1234 56|78", readlen, 8, false, ture);
    //返回value == 0x1234; readlen == 4;
  \endcode
*/
size_t hex2value(const void*    hex,
                 const size_t   size,
                 size_t*        lpreadlen = nullptr,
                 size_t         wantlen   = 0,
                 const bool     errexit   = false,
                 const bool     errbreak  = false);

//! 指定HEX串转换为BIN串
/*!
  转换ASCII的十六进制字符应成对的，如最后字符不成对，将忽略最后不成对的字符
  \param  hex       源HEX串
  \param  size      源串大小
  \param  lpreadlen 成功读取HEX串时，返回读取的字符个数。\n
                    lpreadlen可以为nullptr，此时不返回结果。\n
                    转换失败，*lpreadlen == 0。
  \param  errexit   指定转换未结束前遭遇非HEX字符，是否视作失败处理。\n
                    默认不作失败处理。当此标志为true时，忽略errbreak。
  \param  errbreak  指定转换未结束前遭遇非HEX字符，是否中止处理。默认跳过
  \return           成功转换则为十六进制ASCII串对应的HEX值。\n
                    判定转换失败，应该通过lpreadlen的返回结果判定。
*/
std::string hex2bin(const void*   hex,
                    const size_t  size,
                    size_t*       lpreadlen  = nullptr,
                    const bool    errexit    = false,
                    const bool    errbreak   = false);

//! 指定ASCII串，分析转义字符
/*!
  可转义的字符有：
  \code
    \0、\a、\b、\f、\n、\r、\t、\v、\\、\'、\"、\?
  \endcode
  另可转义\x########。
  \x识别十六进制数据时，根据读取长度，自动匹配类型(2:byte, 4:short, 8:int)
  \param    strs    源ASCII串(转换字串最大长度为0xFFFF)
  \param    strs    源ASCII串大小
  \return           返回转换后的ascii串对象
*/
std::string escape(const void* strs, const size_t size);

enum Hex2showCode
  {
  HC_ASCII,
  HC_UNICODE,
  HC_UTF8,
  };

//! 指定hex串，格式化显示
/*!
  \param  data      hex串
  \param  code      指明内容编码（默认HC_ASCII）
  \param  isup      hex格式大小写控制
  \param  prews     前缀空格数
  \return           格式化后的内容

  \code

    string ss = "123456789aasdfsdhcf";
    cout << hex2show(ss) << endl;
    //0012FF54┃31 32 33 34|35 36 37 38|39 61 61 73|64 66 73 64┃123456789aasdfsd
    //0012FF64┃68 63 66 00|           |           |           ┃hcf.
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

template<typename T>
std::string escape(const std::basic_string<T>&  strs)
  {
  return escape((void*)strs.c_str(), strs.size() * sizeof(T));
  }

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
std::string showbin(const std::basic_string<T>& data, const size_t prews) // 需要强制类型哦
  {
  return showbin((void*)data.c_str(), data.size() * sizeof(T),
                 HC_ASCII, true, prews);
  }

#endif  // _XLIB_HEX_BIN_H_