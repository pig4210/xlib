#include "ws_s.h"

#include <string.h>

using std::string;

#ifdef _WIN32

#include "xlib_nt.h"

string ws2s(const charucs2_t* const ws, const size_t size)
  {
  string s;
  if(ws == nullptr || size == 0)  return s;

  const size_t wlen = size * sizeof(charucs2_t);

  size_t wused = 0;

  while(wused < wlen)
    {
    size_t wnow = wlen - wused;
    if(wnow > 0xFFFC) wnow = 0xFFFC;    // 一次只能处理这么多字符。

    UNICODE_STRING us;
    us.Length = (USHORT)wnow;
    us.MaximumLength = (USHORT)wnow;
    us.Buffer = (PWCH)(ws + wused / sizeof(charucs2_t));

    ANSI_STRING as;
    as.Length = 0;
    as.MaximumLength = 0;
    as.Buffer = nullptr;
    const bool rets = NT_SUCCESS(RtlUnicodeStringToAnsiString(&as, &us, true));
    if(rets && as.Length != 0)
      {
      s.append(as.Buffer, as.Length);
      }
    RtlFreeAnsiString(&as);
    if(!rets)
      {
      break;
      }
    wused += wnow;
    }
  return s;
  }

ucs2string s2ws(const char* const s, const size_t size)
  {
  ucs2string  ws;
  if(s == nullptr || size == 0) return ws;

  const size_t slen = size;

  size_t sused = 0;

  while(sused < slen)
    {
    size_t snow = slen - sused;
    if(snow > 0x7FFE) snow = 0x7FFE;    // 一次只能处理这么多字符。

    ANSI_STRING as;
    as.Length = (USHORT)snow;
    as.MaximumLength = (USHORT)snow;
    as.Buffer = (PCH)(s + sused);

    UNICODE_STRING us;
    us.Length = 0;
    us.MaximumLength = 0;
    us.Buffer = nullptr;
    const bool rets = NT_SUCCESS(RtlAnsiStringToUnicodeString(&us, &as, true));
    if(rets && us.Length != 0)
      {
      ws.append(us.Buffer, us.Length / sizeof(charucs2_t));
      }
    RtlFreeUnicodeString(&us);
    if(!rets)
      {
      break;
      }
    sused += snow;
    }
  return ws;
  }

#else   // _WIN32

#include <iconv.h>

string ws2s(const charucs2_t* const ws, const size_t size)
  {
  string s;
  if(ws == nullptr || size == 0)  return s;

  iconv_t cd = iconv_open(set_ascii_encode(nullptr), "UCS2");
  if(cd == 0) return 0;

  const size_t wlen = size * sizeof(charucs2_t);

  char* const lpnew = new char[wlen];
  char* inbuf = (char*)ws;
  char* outbuf = lpnew;
  size_t inlen = wlen;
  size_t outlen = wlen;

  if(-1 == (intptr_t)iconv(cd, &inbuf, &inlen, &outbuf, &outlen))
    {
    delete[] lpnew;
    iconv_close(cd);
    return s;
    }
  iconv_close(cd);

  outlen = wlen - outlen;

  s.assign(lpnew, outlen);
  delete[] lpnew;
  return s;
  }

ucs2string s2ws(const char* const s, const size_t size)
  {
  ucs2string ws;
  if(s == nullptr || size == 0)  return ws;

  iconv_t cd = iconv_open("UCS2", set_ascii_encode(nullptr));
  if(cd == 0) return 0;

  const size_t slen = size;
  const size_t wlen = slen * sizeof(charucs2_t);

  char* lpnew = new char[wlen];
  char* inbuf = (char*)s;
  char* outbuf = lpnew;
  size_t inlen = slen;
  size_t outlen = wlen;

  if(-1 == (intptr_t)iconv(cd, &inbuf, &inlen, &outbuf, &outlen))
    {
    delete[] lpnew;
    iconv_close(cd);
    return ws;
    }
  iconv_close(cd);

  outlen = (wlen - outlen) / sizeof(charucs2_t);

  ws.assign((charucs2_t*)lpnew, outlen);
  delete[] lpnew;
  return ws;
  }

const char* set_ascii_encode(const char* new_encode)
  {
  static const char* const default_encode = "GB2312";
  static string encode(default_encode);
  if(new_encode == nullptr)
    {
    return encode.c_str();
    }
  if(*new_encode == '\0')
    {
    encode.assign(default_encode);
    return encode.c_str();
    }
  encode.assign(new_encode);
  return encode.c_str();
  }

#endif  // _WIN32

string ws2s(const ucs2string& ws)
  {
  return ws2s(ws.c_str(), ws.size());
  }

ucs2string s2ws(const string& s)
  {
  return s2ws(s.c_str(), s.size());
  }

#ifdef _XLIB_TEST_

ADD_XLIB_TEST(WS_S)
  {
  SHOW_TEST_INIT;

  auto done = false;

  // 测试字符串："AA转换测试BB" 。
  const char* const sbuf = "\x41\x41\xD7\xAA\xBB\xBB\xB2\xE2\xCA\xD4\x42\x42";
  const size_t sbuflen = 12;
  const char* const wbuf = "\x41\x00\x41\x00\x6C\x8F\x62\x63\x4B\x6D\xD5\x8B\x42\x00\x42\x00";
  const size_t wbuflen = 8;
  const size_t wbufreallen = wbuflen * sizeof(charucs2_t);

  SHOW_TEST_HEAD("ws2s");
  const auto s = ws2s(ucs2string((const charucs2_t*)wbuf, wbuflen));
  done = (s.size() == sbuflen) && (0 == memcmp(sbuf, s.c_str(), sbuflen));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("s2ws");
  const auto ws = s2ws(string(sbuf, sbuflen));
  done = (ws.size() == wbuflen) && (0 == memcmp(wbuf, ws.c_str(), wbufreallen));
  SHOW_TEST_RESULT(done);
  }

#endif  // _WIN32