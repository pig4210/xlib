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
#ifndef _XLIB_HEXBIN_H_
#define _XLIB_HEXBIN_H_

#include <string>

/**
  十六进制值的结构体。主要体现 BYTE 与十六进制字符间的关系。

  \code
    BIN_VALUE_STRUCT bins = {1, 4};  // bins 为字符 'A' 。
  \endcode
*/
#pragma pack (push, 1)
struct BIN_VALUE_STRUCT
  {
  uint8_t low:  4;
  uint8_t high: 4;
  };
#pragma pack (pop)

/// 十六进制 ASCII 表现形式的结构体。
struct HEX_VALUE_STRUCT
  {
  int8_t  high;
  int8_t  low;
  };

/**
  指定 BIN 串转换为 HEX(ASCII) 格式。
  \param  bin   源 BIN 串。
  \param  size  源 BIN 串大小。
  \param  isup  指定转换后的 ASCII 大小写，默认小写。
  \return       返回转换后的 ASCII 串对象。

  \code
    string asc = bin2hex(string("\xAB\xCD\xEF"));
    cout << "bin2hex:" << asc ;   // 将输出 "abcdef" 。
  \endcode
*/
template<bool UP> std::string bin2hex(const void* bin, const size_t size)
  {
  std::string as;
  const BIN_VALUE_STRUCT* s = (const BIN_VALUE_STRUCT*)bin;
  const BIN_VALUE_STRUCT* const e = (const BIN_VALUE_STRUCT*)((size_t)bin + size);
  const auto fmt = UP ? "0123456789ABCDEF" : "0123456789abcdef";
  for(; s < e; ++s)
    {
    as.push_back(fmt[s->high]);
    as.push_back(fmt[s->low]);
    }
  return as;
  }
template<typename T> std::string bin2hex(const std::basic_string<T>& bin, const bool isup = false)
  {
  if(isup)
    return bin2hex<true>(bin.c_str(), bin.size() * sizeof(T));
  else
    return bin2hex<false>(bin.c_str(), bin.size() * sizeof(T));
  }

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
template<typename R, typename T>
R hex2value(const std::basic_string<T>& hex,
            size_t*                     lpreadlen = nullptr,
            size_t                      wantlen = 0,
            const bool                  errexit = false,
            const bool                  errbreak = false)
  {
  R values = 0;
  size_t readlen = 0;

  if(lpreadlen == nullptr) lpreadlen = &readlen;
  *lpreadlen = 0;
  
  const auto wl = sizeof(R) * sizeof(HEX_VALUE_STRUCT);

  if((wantlen == 0) || (wantlen > wl))    wantlen = wl;

  for(const auto& ch : hex)
    {
    uint8_t tmpC = ch & 0xF;
    switch(ch)
      {
      case 'A':      case 'B':      case 'C':      case 'D':
      case 'E':      case 'F':
      case 'a':      case 'b':      case 'c':      case 'd':
      case 'e':      case 'f':
        {
        tmpC += 9;  // 注意这里没有 break 。
        }
      case '0':      case '1':      case '2':      case '3':
      case '4':      case '5':      case '6':      case '7':
      case '8':      case '9':
        {
        --wantlen;
        values = values << 0x4;
        values += tmpC;
        if(0 == wantlen)
          {
          ++(*lpreadlen);
          return values;
          }
        break;
        }
      default:
        {
        if(errexit)
          {
          *lpreadlen = 0;
          return 0;
          }
        if(errbreak)  return values;
        break;
        }
      }
    ++(*lpreadlen);
    }
  return values;
  }
template<typename T>
T hex2value(const void* const hex,
            const size_t      size,
            size_t*           lpreadlen = nullptr,
            size_t            wantlen = 0,
            const bool        errexit = false,
            const bool        errbreak = false)
  {
  return hex2value<T>(
    std::string((const char*)hex, (nullptr == hex) ? 0 : size),
    lpreadlen, wantlen, errexit, errbreak);
  }

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

template<typename T>
std::string hex2bin(const std::basic_string<T>& hex,
                    size_t*                     lpreadlen = nullptr,
                    const bool                  errexit = false,
                    const bool                  errbreak = false)
  {
  std::string rets;

  size_t readlen = 0;
  if(lpreadlen == nullptr) lpreadlen = &readlen;
  *lpreadlen = 0;

  bool pick_high = true;      // 指示当前提取的是高位还是低位。
  uint8_t readch = 0;         // 存放临时的提取值。
  size_t realreadlen = 0;     // 存放实际读取数。

  for(const auto& ch : hex)
    {
    uint8_t tmpC = ch & 0xF;
    switch(ch)
      {
      case 'A':      case 'B':      case 'C':      case 'D':
      case 'E':      case 'F':
      case 'a':      case 'b':      case 'c':      case 'd':
      case 'e':      case 'f':
        {
        tmpC += 9;  // 注意这里没有 break 。
        }
      case '0':      case '1':      case '2':      case '3':
      case '4':      case '5':      case '6':      case '7':
      case '8':      case '9':
        {
        if(pick_high)
          {
          readch = tmpC << 0x4;
          }
        else
          {
          readch = (readch & 0xF0) + tmpC;
          rets.push_back(readch);
          realreadlen = *(lpreadlen) + 1;
          }
        pick_high = !pick_high;
        break;
        }
      default:
        {
        if(errexit)
          {
          *lpreadlen = 0;
          rets.clear();
          return rets;
          }
        if(errbreak)
          {
          // 读取不完整。
          if(!pick_high)
            {
            *(lpreadlen) = realreadlen;
            }
          return rets;
          }
        break;
        }
      }
    ++(*lpreadlen);
    }

  return rets;
  }
inline std::string hex2bin(const void*  hex,
                           const size_t size,
                           size_t*      lpreadlen  = nullptr,
                           const bool   errexit    = false,
                           const bool   errbreak   = false)
  {
  return hex2bin(
    std::string((const char*)hex, (nullptr == hex) ? 0 : size),
    lpreadlen, errexit, errbreak);
  }
template<typename T>
std::basic_string<T> escape(const std::basic_string<T>& str)
  {
  auto s = str.c_str();
  auto e = s + str.size();
  std::basic_string<T> ret;
  for(; s < e; ++s)
    {
    if(*s != '\\')
      {
      ret.push_back(*s);
      continue;
      }
    ++s;
    switch(*s)
      {
      case '\0':  ret.push_back('\\'); --s; break;
      case '\'':  ret.push_back('\'');break;
      case '\"':  ret.push_back('\"');break;
      case '\?':  ret.push_back('\?');break;
      case '\\':  ret.push_back('\\');break;
      case 'a':   ret.push_back('\a');  break;
      case 'b':   ret.push_back('\b');  break;
      case 'f':   ret.push_back('\f');  break;
      case 'n':   ret.push_back('\n');  break;
      case 'r':   ret.push_back('\r');  break;
      case 't':   ret.push_back('\t');  break;
      case 'v':   ret.push_back('\v');  break;
      case '0':      case '1':      case '2':      case '3':
      case '4':      case '5':      case '6':      case '7':
        {

        }
        break;
      default:
        break;
      }
    }
  return ret;
  }

#endif  // _XLIB_HEXBIN_H_