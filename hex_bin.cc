#include "hex_bin.h"

#include <string.h>

#include "xmsg.h"

using std::string;

/// 十六进制转换，小写索引。
static const char* const gk_NumFmt_Low  = "0123456789abcdef";
/// 十六进制转换，大写索引。
static const char* const gk_NumFmt_Up   = "0123456789ABCDEF";

string bin2hex(const void* bin, const size_t size, bool isup)
  {
  string asc;
  if(bin == nullptr || size == 0)  return asc;
  const auto fmt = isup ? gk_NumFmt_Up : gk_NumFmt_Low;

  for(size_t i = 0; i < size; ++i)
    {
    const BIN_VALUE_STRUCT* hexchar = (const BIN_VALUE_STRUCT*)((size_t)bin + i);
    asc.push_back(fmt[hexchar->high]);
    asc.push_back(fmt[hexchar->low]);
    }

  return asc;
  }

size_t hex2value(const void*    hex,
                 const size_t   size,
                 size_t*        lpreadlen,
                 size_t         wantlen,
                 const bool     errexit,
                 const bool     errbreak)
  {
  size_t values = 0;
  size_t readlen = 0;

  if(lpreadlen == nullptr) lpreadlen = &readlen;
  *lpreadlen = 0;

  if(hex == nullptr || size == 0)  return 0;

  const auto wl = sizeof(size_t) * sizeof(HEX_VALUE_STRUCT);

  if((wantlen == 0) || (wantlen > wl))    wantlen = wl;

  const char* lp = (const char*)hex;
  for(size_t i = 0; i < size; ++i)
    {
    const uint8 ch = lp[i];
    uint8 tmpC = ch & 0xF;
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
        if(wantlen == 0)
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

string hex2bin(const void*    hex,
               const size_t   size,
               size_t*        lpreadlen,
               const bool     errexit,
               const bool     errbreak)
  {
  string rets;

  size_t readlen = 0;
  if(lpreadlen == nullptr) lpreadlen = &readlen;
  *lpreadlen = 0;

  if(hex == nullptr || size == 0)  return rets;

  bool pick_high = true;      // 指示当前提取的是高位还是低位。
  uint8 readch = 0;           // 存放临时的提取值。
  size_t realreadlen = 0;     // 存放实际读取数。

  const char* lp = (const char*)hex;
  for(size_t i = 0; i < size; ++i)
    {
    const uint8 ch = lp[i];
    uint8 tmpC = ch & 0xF;
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

string escape(const void* strs, const size_t size)
  {
  const char* lp = (const char*)strs;
  string msg;
  for(size_t i = 0; i < size; ++i)
    {
    if(lp[i] != '\\')
      {
      msg.push_back(lp[i]);
      continue;
      }

    switch(lp[++i])
      {
      case '0':   msg.push_back('\0');  break;
      case 'a':   msg.push_back('\a');  break;
      case 'b':   msg.push_back('\b');  break;
      case 'f':   msg.push_back('\f');  break;
      case 'n':   msg.push_back('\n');  break;
      case 'r':   msg.push_back('\r');  break;
      case 't':   msg.push_back('\t');  break;
      case 'v':   msg.push_back('\v');  break;
      case '\\':  msg.push_back('\\');  break;
      case '\'':  msg.push_back('\'');  break;
      case '\"':  msg.push_back('\"');  break;
      case '\?':  msg.push_back('\?');  break;
      case '\0':  msg.push_back('\\'); --i; break;
      case 'x' :
        {
        ++i;
        size_t readlen = 0;
        const size_t tmpI = hex2value((void*)&lp[i], 1, &readlen, 0, false, true);
        switch(readlen)
          {
          case 0:msg.push_back('x');break;
          case 1:
          case 2:msg.append((const char*)&tmpI, sizeof(uint8));break;
          case 3:
          case 4:msg.append((const char*)&tmpI, sizeof(uint16)); break;
#if defined(_WIN64) || defined(__amd64)
          case 5: case 6: case 7: case 8:
            msg.append((const char*)&tmpI, sizeof(uint32));break;
#endif
          default:
            msg.append((const char*)&tmpI, sizeof(tmpI)); break;
          }
        i += (readlen - 1);
        }
        break;
      default:
        msg.push_back('\\');
        msg.push_back(lp[i]);
      }
    }
  return msg;
  }

string escape(const string& strs)
  {
  return escape((void*)strs.c_str(), strs.size());
  }
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
/// 每行显示数据个数。
static const size_t gk_max_line_byte = 0x10;

/// 输出 hex2show 的前缀格式化数据。
static void showbin_prefix(xmsg&          msg,
                           const uint8*   data,
                           const size_t   size,
                           const char*    fmt,
                           const size_t   prews)
  {
  msg.append(prews, ' ');  // 空格前缀。
  msg << (void*)data;      // 地址前缀。
  msg.append("┃");         // 地址与 16 进制数据分隔符。

  for(size_t i = 0; i < gk_max_line_byte; ++i)
    {
    if(i < size)    // HEX 格式化输出。
      {
      msg.push_back(fmt[(data[i] >> 4) & 0xF]);
      msg.push_back(fmt[(data[i] >> 0) & 0xF]);
      }
    else            // 无数据补齐。
      {
      msg.append("  ");
      }
    switch(i)
      {
      case 3:  case 7:  case 11:
        msg.push_back('|'); break;
      case 15:
        msg.append("┃"); break;
      default:
        msg.push_back(' ');
      }
    }
  }

/// 使 UNICODE 字符输出可视化，返回 true 表示接受字符，否则不接受，一律输出 '.' 。
static bool showbin_check_unicode_visualization(xmsg& msg, const charucs2_t wc)
  {
  // 控制字符一律输出 '.' 。
  if(wc < L' ' || (wc >= 0x7F && wc <= 0xA0))
    {
    msg.push_back('.');
    return true;
    }

  // 对于 '?' 做特殊预处理以应对转换无法识别字符。
  if(wc == L'?')
    {
    msg.push_back('?');
    return true;
    }

  // 尝试转换 ASCII ，失败则输出 '.' 。
  const auto ch(ws2s(&wc, 1));
  if(ch.empty())
    {
    msg.push_back('.');
    return false;
    }

  // 转换成功，但无法显示，也输出 '.' 。
  if(*ch.begin() == '?')
    {
    msg.push_back('.');
    return false;
    }

  // 正常输出。
  msg << ch;
  return true;
  }

static size_t showbin_fix_unicode(xmsg&           msg,
                                  const uint8*    data,
                                  const size_t    fix_len,
                                  size_t          used)
  {
  const bool last = (fix_len < gk_max_line_byte);

  while(used < fix_len)
    {
    // 在数据最后一行的最后一个 byte ，无法进行向后匹配完整字符，一律输出 '.' 。
    if(last && (used + 1) >= fix_len)
      {
      msg.push_back('.');
      used += sizeof(char);
      break;
      }

    const charucs2_t wc = *(charucs2_t*)&data[used];

    if(showbin_check_unicode_visualization(msg, wc))
      {
      used += sizeof(charucs2_t);
      }
    else
      {
      used += sizeof(char);
      }
    }
  return used - fix_len;
  }

static size_t showbin_fix_utf8(xmsg&            msg,
                               const uint8*     data,
                               const size_t     fix_len,
                               size_t           used)
  {
  const bool last = (fix_len < gk_max_line_byte);

  while(used < fix_len)
    {
    // 尝试转换 UNICODE 。
    uint32 unicode;
    const size_t k = utf8_byte2unicode_byte(&unicode, (const p_utf8)&data[used]);

    // 转换失败一律输出 '.' 。
    if(k == 0)
      {
      msg.push_back('.');
      used += sizeof(char);
      continue;
      }

    // 在数据最后一行，无法进行向后匹配完整字符，一律输出 '.' 。
    if(last && (used + k) > fix_len)
      {
      msg.append(fix_len - used, '.');
      used = fix_len;
      break;
      }

    // 转换超出正常 UNICODE 范围，一律输出 '.' 。
    if(unicode > 0xFFFF)
      {
      msg.push_back('.');
      used += sizeof(char);
      continue;
      }

    const charucs2_t wc = (charucs2_t)unicode;

    if(showbin_check_unicode_visualization(msg, wc))
      {
      used += k;
      }
    else
      {
      used += sizeof(char);
      }
    }
  return used - fix_len;
  }

static size_t showbin_fix_ascii(xmsg&           msg,
                                const uint8*    data,
                                const size_t    fix_len,
                                size_t          used)
  {
  const bool last = (fix_len < gk_max_line_byte);

  while(used < fix_len)
    {
    const uint8 ch = *(uint8*)&data[used];

    // 控制字符一律输出 '.' 。
    if(ch < ' ' || ch == 0x7F)
      {
      msg.push_back('.');
      used += sizeof(char);
      continue;
      }

    // 可显示字符输出。
    if(ch < 0x7F)
      {
      msg.push_back(ch);
      used += sizeof(char);
      continue;
      }

    // 在数据最后一行的最后一个 byte ，无法进行向后匹配完整字符，一律输出 '.' 。
    if(last && (used + 1) >= fix_len)
      {
      msg.push_back('.');
      used += sizeof(char);
      break;
      }

    // 尝试转换 UNICODE ，转换失败一律输出 '.' 。
    const auto unicode(s2ws(string((const char*)&data[used], sizeof(charucs2_t))));
    if(unicode.empty())
      {
      msg.push_back('.');
      used += sizeof(char);
      continue;
      }

    if(showbin_check_unicode_visualization(msg, *unicode.begin()))
      {
      used += sizeof(charucs2_t);
      }
    else
      {
      used += sizeof(char);
      }
    }

  return used - fix_len;
  }

string showbin(const void*          data,
               const size_t         len,
               const Hex2showCode   code,
               const bool           isup,
               const size_t         prews)
  {
  xmsg msg;
  const char* const fmt = isup ? gk_NumFmt_Up : gk_NumFmt_Low;

  size_t used = 0;
  const uint8* lpdata = (const uint8*)data;
  size_t size = len;

  do 
    {
    showbin_prefix(msg, lpdata, size, fmt, prews);

    size_t fix_len = size > gk_max_line_byte ? gk_max_line_byte : size;

    switch(code)
      {
      case HC_UNICODE:
        used = showbin_fix_unicode(msg, lpdata, fix_len, used);
        break;
      case HC_UTF8:
        used = showbin_fix_utf8(msg, lpdata, fix_len, used);
        break;
      case HC_ASCII:
      default:
        used = showbin_fix_ascii(msg, lpdata, fix_len, used);
      }

    lpdata += gk_max_line_byte;
    msg.push_back('\r');
    msg.push_back('\n');

    size -= fix_len;
    }while(size != 0);

  return msg;
  }

#ifdef _XLIB_TEST_

ADD_XLIB_TEST(HEX_BIN)
  {
  SHOW_TEST_INIT;

  auto done = false;

  const char* const buf = "12345678";
  const char* const hex = "3132333435363738";
  string rets;

  SHOW_TEST_HEAD("bin2hex");
  done = (bin2hex(string(buf)) == hex);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("hex2value");
  done = (0x12345678 == hex2value(string(buf)));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("hex2bin");
  done = (string(buf) == hex2bin(string(hex)));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("showbin");
  rets = showbin(string("0123456789ABCDEF"));
  rets.erase(rets.begin(), rets.begin() + sizeof(void*) * sizeof(HEX_VALUE_STRUCT));
  done = (rets == "┃30 31 32 33|34 35 36 37|38 39 41 42|43 44 45 46┃0123456789ABCDEF\r\n");
  SHOW_TEST_RESULT(done);
  }

#endif  // _XLIB_TEST_