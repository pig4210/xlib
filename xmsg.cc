#include "xmsg.h"

#include <stdio.h>
#include <stdarg.h>

using std::string;
using std::basic_string;
using ucs2string;

xmsg::xmsg()
  {
  ;
  }

xmsg::xmsg(const string& s)
  {
  assign(s);
  }

xmsg::xmsg(const ucs2string& s)
  {
  assign(ws2s(s));
  }

#ifdef _WIN32
#   pragma warning(push)
#   pragma warning(disable:4127)  //warning C4127: 条件表达式是常量
#endif
xmsg& xmsg::prt(const char* const fmt, ...)
  {
  va_list ap;
  va_start(ap, fmt);
  while(true)
    {
    if(capacity() - size() <= 1)
      {
      reserve(capacity() + 0x10);
      }
    const size_t rst = capacity() - size();
#ifdef _WIN32
#   ifdef FOR_RING0
    char* lpend = end();
#   else
    char* lpend = const_cast<char*>(end()._Ptr);
#   endif
#else
    char* lpend = const_cast<char*>(c_str()) + size();
#endif
#ifdef _WIN32
#   if defined(FOR_RING0) && !defined(_WIN64)
    //目前WIN7以下Ring0只能使用不安全的函数，否则编译出现链接错误
    const size_t rt = _vsnprintf(lpend, rst, fmt, ap);  //数据不足，返回-1
#   else
    const size_t rt = _vsnprintf_s(lpend, rst, rst - 1, fmt, ap);
#   endif
#else
    const size_t rt = vsnprintf(lpend, rst, fmt, ap);  //数据不足，返回-1
#endif
    if(rt < rst)
      {
      append(lpend, rt);
      break;
      }
    reserve(capacity() + 0x10);
    }
  va_end(ap);
  return *this;
  }
#ifdef _WIN32
#   pragma warning(pop)
#endif

xmsg& xmsg::operator<<(const void* v)
  {
#ifdef _WIN32
  return prt("%p", v);
#else     //g++输出%p不填充0，故不采用%p
#   ifdef __amd64
  return operator<<((uint64)v);
#   else
  return operator<<((uint32)v);
#   endif
#endif
  }

xmsg& xmsg::operator<<(const bool& v)
  {
  const char* const tmp = v ? ":true" : ":false";
  return this->operator <<(tmp);
  }

xmsg& xmsg::operator<<(const char& v)
  {
  return prt("%c", v);
  }

xmsg& xmsg::operator<<(const char* v)
  {
  return prt("%s", v);
  }

xmsg& xmsg::operator<<(const charucs2_t& v)
  {
  return prt("%s", ws2s(ucs2string(1, v)).c_str());
  }

xmsg& xmsg::operator<<(const charucs2_t* v)
  {
  return prt("%s", ws2s(v).c_str());
  }

xmsg& xmsg::operator<<(const float& v)
  {
  return prt("%f", v);
  }

xmsg& xmsg::operator<<(const double& v)
  {
  return prt("%f", v);
  }

xmsg& xmsg::operator<<(const string& v)
  {
  return prt("%s", v.c_str());
  }

xmsg& xmsg::operator<<(const ucs2string& v)
  {
  return this->operator<<(v.c_str());
  }

xmsg& xmsg::operator<<(const xutf8& v)
  {
  return this->operator<<(utf82ws(v));
  }

xmsg& xmsg::operator<<(xmsg& (*pfn)(xmsg&))
  {
  return pfn(*this);
  }

xmsg& xmsg::operator<<(const uint8& v)
  {
  return prt("%02X", v);
  }

xmsg& xmsg::operator<<(const int16& v)
  {
  //在g++下，需要两个参数才正常，原因不明
  return prt("%hd", v, v);
  }

xmsg& xmsg::operator<<(const uint16& v)
  {
  return prt("%04X", v);
  }

xmsg& xmsg::operator<<(const int32& v)
  {
  return prt("%d", v);
  }

xmsg& xmsg::operator<<(const uint32& v)
  {
  return prt("%08X", v);
  }

xmsg& xmsg::operator<<(const int64& v)
  {
  return prt("%lld", v);
  }

xmsg& xmsg::operator<<(const uint64& v)
  {
  return prt("%08X%08X", (uint32)(v >> 32), (uint32)v);
  }


#ifdef _XLIB_TEST_

ADD_XLIB_TEST(XMSG)
  {
  SHOW_TEST_INIT;

  auto done = false;

  xmsg msg;

  SHOW_TEST_HEAD("string");
  msg = xmsg("123");
  done = (msg == string("123"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("ucs2string");
  msg = xmsg(s2ws("123"));
  done = (msg == string("123"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("void*");
  msg.clear();
  msg << (void*)0x12345678;
#if defined(_WIN64) || defined(__amd64)
  done = (msg == string("0000000012345678"));
#else
  done = (msg == string("12345678"));
#endif
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("bool");
  msg.clear();
  msg << true;
  done = (msg == string(":true"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("char");
  msg.clear();
  msg << '1';
  done = (msg == string("1"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("char*");
  msg.clear();
  msg << "123";
  done = (msg == string("123"));
  SHOW_TEST_RESULT(done);


  SHOW_TEST_HEAD("charucs2_t");
  msg.clear();
  msg << (charucs2_t)0x31;
  done = (msg == string("1"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("charucs2_t*");
  msg.clear();
  msg << s2ws("123").c_str();
  done = (msg == string("123"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("float");
  msg.clear();
  msg << (float)1.0;
  done = (msg == string("1.000000"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("double");
  msg.clear();
  msg << (double)1.0;
  done = (msg == string("1.000000"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("string");
  msg.clear();
  msg << string("123");
  done = (msg == string("123"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("ustring");
  msg.clear();
  msg << ucs2string(s2ws("123"));
  done = (msg == string("123"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("xutf8");
  msg.clear();
  msg << ws2utf8(s2ws(string("\x41\x41\xD7\xAA\xBB\xBB\xB2\xE2\xCA\xD4\x42\x42")));
  done = (msg == string("\x41\x41\xD7\xAA\xBB\xBB\xB2\xE2\xCA\xD4\x42\x42"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("uint8");
  msg.clear();
  msg << (uint8)'1';
  done = (msg == string("31"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("int16");
  msg.clear();
  msg << (int16)1234;
  done = (msg == string("1234"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("uint16");
  msg.clear();
  msg << (uint16)0x1234;
  done = (msg == string("1234"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("int32");
  msg.clear();
  msg << (int32)1234;
  done = (msg == string("1234"));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("uint32");
  msg.clear();
  msg << (uint32)0x12345678;
  done = (msg == string("12345678"));
  SHOW_TEST_RESULT(done);
  }

#endif  // _XLIB_TEST_