#include "ws_utf8.h"

#include <string.h>
 
static const size_t gk_utf8_max_byte = 6;   // UTF8 最大占用字节。

#ifdef _WIN32
#   pragma warning(push)
#   pragma warning(disable:4244) // “=”: 从“const uint32”转换到“uint8”，可能丢失数据。
#endif
xutf8 unicode_byte2utf8_byte(const uint32 unicode)
  {
  xutf8 rets;
  // 0000 0000 0000 0000 0000 0000 0111 1111     7bit
  // 0000 0000 0000 0000 0000 0000 0XXX XXXX     7bit
  if(unicode < 0x00000080)
    {
    rets.insert(rets.begin(), ((unicode & 0x0000007F) >> 0) | 0x00);
    return rets;
    }
  // 0000 0000 0000 0000 0000 0000 10XX XXXX     6bit
  rets.insert(rets.begin(), ((unicode & 0x0000003F) >> 0) | 0x80);
  // 0000 0000 0000 0000 0000 0111 1111 1111     11bit
  // 0000 0000 0000 0000 0011 0XXX XX00 0000     5bit
  if(unicode < 0x00000800)
    {
    rets.insert(rets.begin(), ((unicode & 0x000007C0) >> 6) | 0xC0);
    return rets;
    }
  // 0000 0000 0000 0000 0010 XXXX XX00 0000     6bit
  rets.insert(rets.begin(), ((unicode & 0x00000FC0) >> 6) | 0x80);
  // 0000 0000 0000 0000 1111 1111 1111 1111     16bit
  // 0000 0000 0000 1110 XXXX 0000 0000 0000     4bit
  if(unicode < 0x00010000)
    {
    rets.insert(rets.begin(), ((unicode & 0x0000F000) >> 12) | 0xE0);
    return rets;
    }
  // 0000 0000 0000 10XX XXXX 0000 0000 0000     6bit
  rets.insert(rets.begin(), ((unicode & 0x0003F000) >> 12) | 0x80);
  // 0000 0000 0001 1111 1111 1111 1111 1111     21bit
  // 0000 0011 110X XX00 0000 0000 0000 0000     3bit
  if(unicode < 0x00200000)
    {
    rets.insert(rets.begin(), ((unicode & 0x001C0000) >> 18) | 0xF0);
    return rets;
    }
  // 0000 0010 XXXX XX00 0000 0000 0000 0000     6bit
  rets.insert(rets.begin(), ((unicode & 0x00FC0000) >> 18) | 0x80);
  // 0000 0011 1111 1111 1111 1111 1111 1111     26bit
  // 1111 10XX 0000 0000 0000 0000 0000 0000     2bit
  if(unicode < 0x04000000)
    {
    rets.insert(rets.begin(), ((unicode & 0x03000000) >> 24) | 0xF8);
    return rets;
    }
  // 00XX XXXX 0000 0000 0000 0000 0000 0000     6bit
  rets.insert(rets.begin(), ((unicode & 0x3F000000) >> 24) | 0x80);
  // 0111 1111 1111 1111 1111 1111 1111 1111     31bit
  // 0X00 0000 0000 0000 0000 0000 0000 0000     1bit
  if(unicode < 0x80000000)
    {
    rets.insert(rets.begin(), ((unicode & 0x04000000) >> 30) | 0xFC);
    return rets;
    }
  return rets;
  }
#ifdef _WIN32
#   pragma warning(pop)
#endif

size_t utf8_byte2unicode_byte(uint32* unicode, pc_utf8 const utf8)
  {
  if(utf8 == nullptr)  return 0;
  uint32 tu;
  if(unicode == nullptr) unicode = &tu;

  const uint8 utf8_flag[gk_utf8_max_byte] = { 0x7F, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

  if(utf8[0] <= utf8_flag[0])
    {
    *unicode = utf8[0];
    return 1;
    }

  if(utf8[0] < utf8_flag[1])   // 首字节非法。
    {
    return 0;
    }

  size_t lp = 0;

  for(size_t i = 2; i < sizeof(utf8_flag); ++i)
    {
    // 判定首字节处于哪个区域。
    if((utf8[lp] < utf8_flag[i]))
      {
      uint32 u = utf8[lp] ^ utf8_flag[i - 1];
      ++lp;
      for(size_t j = 1; j < i; ++j)
        {
        if(utf8[lp] >= utf8_flag[1])  return 0; // 后继字节非法，跳过。
        u <<= 6;
        u |= (utf8[lp] & 0x3F);
        ++lp;
        }
      *unicode = u;
      return lp;
      }
    }
  return 0;
  }

xutf8 ws2utf8(const charucs2_t* const ws, const size_t size)
  {
  xutf8 utf8;
  if(ws == nullptr || size == 0)  return utf8;

  for(size_t i = 0; i < size; ++i)
    {
    const auto k = unicode_byte2utf8_byte(ws[i]);
    if(k.empty())
      {
      utf8.clear();
      break;
      }
    utf8.append(k);
    }

  return utf8;
  }

xutf8 ws2utf8(const ucs2string& ws)
  {
  return ws2utf8(ws.c_str(), ws.size());
  }

ucs2string utf82ws(pc_utf8 const utf8, const size_t size)
  {
  ucs2string ws;
  if(utf8 == nullptr || size == 0)  return ws;

  pc_utf8 pu = utf8;
  uint32 ch;

  for(size_t i = 0; i < size;)
    {
    const size_t k = utf8_byte2unicode_byte(&ch, &pu[i]);
    if(k == 0)
      {
      ws.clear();
      return ws;
      }

    i += k;
    if(i > size)  break;

    ws.push_back((charucs2_t)ch);
    }

  return ws;
  }

ucs2string utf82ws(const xutf8& utf8)
  {
  return utf82ws(utf8.c_str(), utf8.size());
  }

#ifdef _XLIB_TEST_

ADD_XLIB_TEST(WS_UTF8)
  {
  SHOW_TEST_INIT;

  auto done = false;

  // 测试字符串：“AA转换测试BB” 。
  const xutf8 sbuf((const p_utf8)"\x41\x41\xE8\xBD\xAC\xE6\x8D\xA2\xE6\xB5\x8B\xE8\xAF\x95\x42\x42");
  const ucs2string wbuf((const charucs2_t*)"\x41\x00\x41\x00\x6C\x8F\x62\x63\x4B\x6D\xD5\x8B\x42\x00\x42\x00\x00\x00");

  SHOW_TEST_HEAD("unicode_byte2utf8_byte");
  const auto s = unicode_byte2utf8_byte(0x8F6C);
  done = (s.size() == 3) && (0 == memcmp("\xE8\xBD\xAC", s.c_str(), 3));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("utf8_byte2unicode_byte");
  uint32 unicode = 0;
  const auto retlen = utf8_byte2unicode_byte(&unicode, (const p_utf8)"\xE8\xBD\xAC");
  done = (retlen == 3) && (0x8F6C == unicode);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("ws2utf8");
  done = (sbuf == ws2utf8(wbuf));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("utf82ws");
  done = (wbuf == utf82ws(sbuf));
  SHOW_TEST_RESULT(done);
  }

#endif  // _XLIB_TEST_