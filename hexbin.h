﻿/**
  \file  hex_bin.h
  \brief 定义了 hex 与 bin 的转换操作。

  \version    3.0.0.200313
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
  - 2019-10-14 重构。 2.0 。
  - 2019-11-06 改进定义。 2.1 。
  - 2020-03-13 改进 showbin 返回 wstring 。 3.0 。
*/
#ifndef _XLIB_HEXBIN_H_
#define _XLIB_HEXBIN_H_

#include <climits>
#include <string>

#include "xswap.h"
#include "as_ws_u8.h"

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
  指定 BIN 串转换为 HEX 格式。
  \param  bin   源 BIN 串。
  \param  size  源 BIN 串大小（以类型字大小计）。
  \param  isup  指定转换后的 HEX 大小写，默认小写。
  \return       返回转换后的 HEX 串对象。

  \code
    std::cout << "bin2hex:" << bin2hex(string("\xAB\xCD\xEF"));   // 将输出 "abcdef" 。
  \endcode
*/
template<typename T>
std::string bin2hex(const T* const bin, const size_t size, const bool isup = false)
  {
  std::string hex;
  const BIN_VALUE_STRUCT* s = (const BIN_VALUE_STRUCT*)bin;
  const BIN_VALUE_STRUCT* const e = (const BIN_VALUE_STRUCT*)(bin + size);
  const auto fmt = isup ? "0123456789ABCDEF" : "0123456789abcdef";
  for(; s < e; ++s)
    {
    hex.push_back(fmt[s->high]);
    hex.push_back(fmt[s->low]);
    }
  return hex;
  }
inline std::string bin2hex(const void* const bin, const size_t size, const bool isup = false)
  {
  return bin2hex((const char*)bin, size, isup);
  }
template<typename T> std::string bin2hex(const std::basic_string<T>& bin, const bool isup = false)
  {
  return bin2hex(bin.c_str(), bin.size(), isup);
  }

/**
  指定 HEX 串转换为值。
  \param  hex       源 HEX 串。
  \param  size      源串大小（以类型字大小计）。
  \param  lpreadlen 成功读取 HEX 串时，返回读取的字符个数。\n
                    lpreadlen 可以为 nullptr ，此时不返回结果。\n
                    转换失败， *lpreadlen == 0 。
  \param  wantlen   需要处理的 \b 有效数据 大小 （ 0 < wantlen <= sizeof(R) * 2 ）。\n
                    超出范围，一律视 == sizeof(R) * 2 。
  \param  errexit   指定转换未结束前遭遇非 HEX 字符，是否视作失败处理。\n
                    默认不作失败处理。当此标志为 true 时，忽略 errbreak 。
  \param  errbreak  指定转换未结束前遭遇非 HEX 字符，是否中止处理。默认跳过。
  \return           成功转换则为 HEX 串对应的 数值。\n
                    判定转换失败，应该通过 lpreadlen 的返回结果判定。

  \code
    size_t readlen = 0;
    const auto value = hex2value<size_t>("12345678", readlen);
    // 返回 value == 0x12345678; readlen == 8;
    const auto value = hex2value<size_t>("1234 56|78", readlen);
    // 返回 value == 0x12345678; readlen == 10;
    const auto value = hex2value<size_t>("1234 56|78", readlen, 8, true);
    // 返回 value == 0; readlen == 0;
    const auto value = hex2value<size_t>("1234 56|78", readlen, 8, false, ture);
    // 返回 value == 0x1234; readlen == 4;
  \endcode
*/
template<typename R, typename T>
std::enable_if_t<std::is_integral<R>::value, R>
  hex2value(const T* const  hex,
            const size_t    size,
            size_t*         lpreadlen = nullptr,
            size_t          wantlen = 0,
            const bool      errexit = false,
            const bool      errbreak = false)
  {
  R values = 0;
  size_t readlen = 0;

  if(lpreadlen == nullptr) lpreadlen = &readlen;
  *lpreadlen = 0;
  
  const auto wl = sizeof(R) * sizeof(HEX_VALUE_STRUCT);

  if((wantlen == 0) || (wantlen > wl))    wantlen = wl;

  for(size_t i = 0; i < size; ++i)
    {
    const auto& ch = hex[i];
    uint8_t tmpC = ch & 0xF;
    switch(ch)
      {
      case 'A':      case 'B':      case 'C':      case 'D':
      case 'E':      case 'F':
      case 'a':      case 'b':      case 'c':      case 'd':
      case 'e':      case 'f':
        tmpC += 9;  // 注意这里没有 break 。
      case '0':      case '1':      case '2':      case '3':
      case '4':      case '5':      case '6':      case '7':
      case '8':      case '9':
        --wantlen;
        values = values << 0x4;
        values += tmpC;
        if(0 == wantlen)
          {
          ++(*lpreadlen);
          return values;
          }
        break;
      default:
        if(errexit)
          {
          *lpreadlen = 0;
          return 0;
          }
        if(errbreak)  return values;
        break;
      }
    ++(*lpreadlen);
    }
  return values;
  }

template<typename R>
std::enable_if_t<std::is_integral<R>::value, R>
  hex2value(const void* const   hex,
            const size_t        size,
            size_t*             lpreadlen = nullptr,
            size_t              wantlen = 0,
            const bool          errexit = false,
            const bool          errbreak = false)
  {
  return hex2value<R>((const char*)hex, size, lpreadlen, wantlen, errexit, errbreak);
  }
template<typename R, typename T>
std::enable_if_t<std::is_integral<R>::value, R>
  hex2value(const std::basic_string<T>& hex,
            size_t*                     lpreadlen = nullptr,
            size_t                      wantlen = 0,
            const bool                  errexit = false,
            const bool                  errbreak = false)
  {
  return hex2value<R>(hex.c_str(), hex.size(), lpreadlen, wantlen, errexit, errbreak);
  }

/**
  指定 HEX 串转换为 BIN 串。
  
  HEX 字符应成对，如最后字符不成对，将忽略最后不成对的字符。
  \param  hex       源 HEX 串。
  \param  size      源串大小（以字类型大小计）。
  \param  lpreadlen 成功读取 HEX 串时，返回读取的字符个数。\n
                    lpreadlen 可以为 nullptr ，此时不返回结果。\n
                    转换失败， *lpreadlen == 0。
  \param  errexit   指定转换未结束前遭遇非 HEX 字符，是否视作失败处理。\n
                    默认不作失败处理。当此标志为 true 时，忽略 errbreak 。
  \param  errbreak  指定转换未结束前遭遇非 HEX 字符，是否中止处理。默认跳过。
  \return           成功转换则为 HEX 串对应的 BIN 数据。\n
                    判定转换失败，应该通过 lpreadlen 的返回结果判定。
*/
template<typename T>
std::string hex2bin(const T* const  hex,
                    const size_t    size,
                    size_t*         lpreadlen = nullptr,
                    const bool      errexit = false,
                    const bool      errbreak = false)
  {
  std::string rets;

  size_t readlen = 0;
  if(lpreadlen == nullptr) lpreadlen = &readlen;
  *lpreadlen = 0;

  bool pick_high = true;      // 指示当前提取的是高位还是低位。
  uint8_t readch = 0;         // 存放临时的提取值。
  size_t realreadlen = 0;     // 存放实际读取数。

  for(size_t i = 0; i < size; ++i)
    {
    const auto& ch = hex[i];
    uint8_t tmpC = ch & 0xF;
    switch(ch)
      {
      case 'A':      case 'B':      case 'C':      case 'D':
      case 'E':      case 'F':
      case 'a':      case 'b':      case 'c':      case 'd':
      case 'e':      case 'f':
        tmpC += 9;  // 注意这里没有 break 。
      case '0':      case '1':      case '2':      case '3':
      case '4':      case '5':      case '6':      case '7':
      case '8':      case '9':
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
      default:
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
    ++(*lpreadlen);
    }

  return rets;
  }
template<typename T>
std::string hex2bin(const void* const   hex,
                    const size_t        size,
                    size_t*             lpreadlen = nullptr,
                    const bool          errexit = false,
                    const bool          errbreak = false)
  {
  return hex2bin((const char*)hex, size, lpreadlen, errexit, errbreak);
  }
template<typename T>
std::string hex2bin(const std::basic_string<T>& hex,
                    size_t*                     lpreadlen = nullptr,
                    const bool                  errexit = false,
                    const bool                  errbreak = false)
  {
  return hex2bin(hex.c_str(), hex.size(), lpreadlen, errexit, errbreak);
  }

/**
  指定分析转义字符。

  可转义的字符及相关说明参考 <https://zh.cppreference.com/w/cpp/language/escape> 。

  注意： \u \U 不强制长度，不进行对应编码转换。
  
  \param    str     源字符串。
  \param    size    源字符串大小（以字类型大小计）。
  \return           返回转换后的字符串。
*/
template<typename T>
std::basic_string<T> escape(const T* const str, const size_t size)
  {
  const T* s = str;
  const T* const e = s + size;
  std::basic_string<T> ret;
  for(; s < e; ++s)
    {
    if(*s != '\\')
      {
      ret.push_back(*s);
      continue;
      }
    ++s;
    if(s >= e)
      {
      ret.push_back('\\');
      break;
      }
    switch(*s)
      {
      case '\'':  ret.push_back('\'');  break;
      case '\"':  ret.push_back('\"');  break;
      case '\?':  ret.push_back('\?');  break;
      case '\\':  ret.push_back('\\');  break;
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
        uint32_t tmpC = 0;
        for(auto i = 0; i < 3; ++i)
          {
          if((s >= e) || (*s < '0') || (*s > '7')) break;
          const auto ch = tmpC * 8 + (*s & 0x7);
          if(0x100 <= ch) break;
          tmpC = ch;
          ++s;
          }
        ret.push_back((uint8_t)tmpC);
        --s;
        }
        break;
      case 'x':
        {
        size_t readed = 0;
        ++s;
        const auto v = hex2value<T>(s, e - s, &readed, 0, true, true);
        ret.push_back((0 == readed) ? 'x' : v);
        s += readed;
        --s;
        }
        break;
      case 'u':
        {
        size_t readed = 0;
        ++s;
        const auto v = hex2value<char16_t>(s, e - s, &readed, 0, true, true);
        if(0 == readed)
          {
          ret.push_back('u');
          }
        else
          {
          constexpr size_t x = sizeof(v) / sizeof(T);
          if constexpr (0 == x)
            {
            ret.push_back((T)v);
            }
          else
            {
            for(size_t i = 0; i < x; ++i)
              {
              ret.push_back(((const T*)&v)[i]);
              }
            }
          }
        s += readed;
        --s;
        }
        break;
      case 'U':
        {
        size_t readed = 0;
        ++s;
        const auto v = hex2value<char32_t>(s, e - s, &readed, 0, true, true);
        if(0 == readed)
          {
          ret.push_back('U');
          }
        else
          {
          constexpr size_t x = sizeof(v) / sizeof(T);
          if constexpr (0 == x)
            {
            ret.push_back((T)v);
            }
          else
            {
            for(size_t i = 0; i < x; ++i)
              {
              ret.push_back(((const T*)&v)[i]);
              }
            }
          }
        s += readed;
        --s;
        }
        break;
      default:    ret.push_back('\\'); --s; break;
      }
    }
  return ret;
  }
inline std::string escape(const void* const str, const size_t size)
  {
  return escape((std::string::const_pointer)str, size);
  }
template<typename T>
std::basic_string<T> escape(const std::basic_string<T>& str)
  {
  return escape(str.c_str(), str.size());
  }

enum ShowBinCode
  {
  SBC_ANSI,
  SBC_UNICODE,
  SBC_UTF8
  };

constexpr bool LocaleCheck()
  {
  return (uint8_t)*u8"文" != (uint8_t)*"文";
  }

constexpr ShowBinCode DefShowBinCode = LocaleCheck() ? SBC_ANSI : SBC_UTF8;

/**
  指定 BIN 串，格式化显示。
  \param  bin       BIN 串。
  \param  len       BIN 串长度。
  \param  code      指明内容编码（默认编码自动选择 ANSI 或 UTF8 ）。
  \param  isup      HEX 格式大小写控制。
  \param  prews     前缀空格数。
  \return           格式化后的内容。

  \code
    std::string ss = "123456789aasdfsdhcf";
    std::wcout << showbin(ss);
    // 0012FF54┃31 32 33 34|35 36 37 38|39 61 61 73|64 66 73 64┃123456789aasdfsd
    // 0012FF64┃68 63 66 00|           |           |           ┃hcf.
  \endcode
*/
template<typename T>
std::wstring showbin(const T* const    bin,
                     const size_t      len,
                     const ShowBinCode code  = DefShowBinCode,
                     const bool        isup  = true,
                     const size_t      prews = 0)
  {
  std::wstring ret;
  const auto fmt = isup ? L"0123456789ABCDEF" : L"0123456789abcdef";

  size_t used = 0;
  const uint8_t* data = (const uint8_t*)bin;
  size_t size = len * sizeof(T);

  // 每行显示数据个数。
  constexpr size_t k_max_line_byte = 0x10;

  // 输出 前缀格式化数据。
  auto prefix = [&]
    {
    ret.append(prews, L' ');  // 空格前缀。
    const auto p = bswap((size_t)data);
    ret.append(as2ws(bin2hex(&p, 1)));  // 地址前缀。
    ret.append(L" |");
    
    for(size_t i = 0; i < k_max_line_byte; ++i)
      {
      if(i < size)    // HEX 格式化输出。
        {
        ret.push_back(fmt[(data[i] >> 4) & 0xF]);
        ret.push_back(fmt[(data[i] >> 0) & 0xF]);
        }
      else            // 无数据补齐。
        {
        ret.append(L"  ");
        }
      switch(i)
        {
        case 3:  case 7:  case 11:
          ret.push_back(L'|'); break;
        case 15:
          ret.append(L"| "); break;
        default:
          ret.push_back(L' ');
        }
      }
    };

  // 使 UNICODE 字符输出可视化，返回 true 表示接受字符，否则不接受，一律输出 '.' 。
  auto check_unicode_visualization = [](const wchar_t wc) -> std::wstring
    {
    // 控制字符一律输出 '.' 。
    if(wc < L' ' || (wc >= 0x7F && wc <= 0xA0))
      {
      return L".";
      }

    // 尝试转换 可视化。
    const auto ch(std::wstring(1, wc));
    if constexpr (LocaleCheck())
      {
      return ws2as(ch).empty() ? std::wstring() : ch;
      }
    else
      {
      return ws2u8(ch).empty() ? std::wstring() : ch;
      }
    };

  using checkfunc = std::wstring (*)(const wchar_t);
  using fixfunc = std::wstring (*)(const void* const data, size_t& used, const size_t size, checkfunc check);
  
  fixfunc fix_unicode = [](const void* const data, size_t& used, const size_t size, checkfunc check) -> std::wstring
    {
    // 无法进行向后匹配完整字符。
    if((used + sizeof(wchar_t)) > size)
      {
      return L"";
      }

    const auto ws = check(*(wchar_t*)((const uint8_t*)data + used));

    if(ws.empty())  return ws;

    used += sizeof(wchar_t);
    return ws;
    };
  
  fixfunc fix_ansi = [](const void* const data, size_t& used, const size_t size, checkfunc check) -> std::wstring
    {
    for(size_t i = 1; i <= 2; ++i)
      {
      // 无法进行向后匹配完整字符。
      if(used + i > size) break;
      const auto ws = as2ws(std::string((const char*)data + used, i));
      // 转换失败，尝试扩展匹配。
      if(ws.empty()) continue;

      const auto s = check(*ws.begin());
      // 可视化失败，返回。
      if(s.empty())  break;
      used += i;
      return s;
      }
    return L"";
    };

  auto fix_utf8 = [](const void* const data, size_t& used, const size_t size, checkfunc check) -> std::wstring
    {
    for(size_t i = 1; i <= 6; ++i)
      {
      // 无法进行向后匹配完整字符。
      if(used + i > size) break;
      const auto ws = u82ws(std::u8string((const char8_t*)data + used, i));
      // 转换失败，尝试扩展匹配。
      if(ws.empty()) continue;
      const auto s = check(*ws.begin());
      // 可视化失败，返回。
      if(s.empty())  break;
      used += i;
      return s;
      }
    return L"";
    };
  
  fixfunc fix = (SBC_UNICODE == code) ? fix_unicode : ((SBC_UTF8 == code) ? fix_utf8 : fix_ansi );

  do
    {
    prefix();
    const size_t fix_len = std::min(size, k_max_line_byte);

    while(used < fix_len)
      {
      const auto ws = fix(data, used, size, check_unicode_visualization);
      if(ws.empty())
        {
        used += sizeof(char);
        ret.push_back(L'.');
        }
      else
        {
        ret.append(ws);
        }
      }
    used -= fix_len;
    ret.push_back(L'\r');
    ret.push_back(L'\n');

    data += k_max_line_byte;
    size -= fix_len;
    }while(size != 0);
  return ret;
  }

inline
std::wstring showbin(const void* const bin,
                     const size_t      len,
                     const ShowBinCode code  = DefShowBinCode,
                     const bool        isup  = true,
                     const size_t      prews = 0)
  {
  return showbin((const char*)bin, len, code, isup, prews);
  }

template<typename T>
std::wstring showbin(const std::basic_string<T>& data,
                     const ShowBinCode           code,
                     const bool                  isup,
                     const size_t                prews)
  {
  return showbin(data.c_str(), data.size(), code, isup, prews);
  }
  
template<typename T>
std::wstring showbin(const std::basic_string<T>& data)
  {
  return showbin(data.c_str(), data.size(), DefShowBinCode, true, 0);
  }

template<typename T>
std::wstring showbin(const std::basic_string<T>& data, const ShowBinCode code)
  {
  return showbin(data.c_str(), data.size(), code, true, 0);
  }

template<typename T>
std::wstring showbin(const std::basic_string<T>& data, const bool isup)
  {
  return showbin(data.c_str(), data.size(), DefShowBinCode, true, 0);
  }

template<typename T>
std::wstring showbin(const std::basic_string<T>& data, const size_t prews) // 需要强制类型哦。
  {
  return showbin(data.c_str(), data.size(), DefShowBinCode, true, prews);
  }

#endif  // _XLIB_HEXBIN_H_