#include "xmsg.h"

#include "xlib_test.h"

#define XMSGS(text) std::u8string((const char8_t*)u8 ## text)
#define XMSGWS(v) ws2u8(v)

SHOW_TEST_INIT(XMSG)

SHOW_TEST_HEAD(constructor as);
done = xmsg(std::string("AA\xD7\xAA\xBB\xBB\xB2\xE2\xCA\xD4\x42\x42")) == XMSGWS(L"AA转换测试BB");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(constructor ws);
done = xmsg(L"AA转换测试BB") == XMSGWS(L"AA转换测试BB");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(constructor u8);
done = xmsg(XMSGS("AA转换测试BB")) == XMSGWS(L"AA转换测试BB");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(prt);
done = xmsg().prt("%s", "AA\xD7\xAA\xBB\xBB\xB2\xE2\xCA\xD4\x42\x42") == XMSGWS(L"AA转换测试BB");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(int8_t);
done = (xmsg() << (int8_t)-1) == XMSGS("-1");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(uint8_t);
done = (xmsg() << (uint8_t)1) == XMSGS("01");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(int16_t);
done = (xmsg() << (int16_t)-1) == XMSGS("-1");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(uint16_t);
done = (xmsg() << (uint16_t)255) == XMSGS("00FF");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(int32_t);
done = (xmsg() << (int32_t)-2) == XMSGS("-2");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(uint32_t);
done = (xmsg() << (uint32_t)4294967294) == XMSGS("FFFFFFFE");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(int64_t);
done = (xmsg() << (int64_t)0x112210F47DE98115) == XMSGS("1234567890123456789");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(uint64_t);
done = (xmsg() << (uint64_t)1234567890123456789) == XMSGS("112210F47DE98115");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(void*);
#if defined(_WIN64) || defined(__amd64)
  done = (xmsg() << (void*)0x12345678) == XMSGS("0000000012345678");
#else
  done = (xmsg() << (void*)0x12345678) == XMSGS("12345678");
#endif
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(bool);
done = (xmsg() << true) == XMSGS("true");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(char);
done = (xmsg() << '1') == XMSGS("1");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(char*);
done = (xmsg() << "123") == XMSGS("123");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(string);
done = (xmsg() << std::string("123")) == XMSGS("123");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(wchar_t);
done = (xmsg() << L'1') == XMSGS("1");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(wchar_t*);
done = (xmsg() << L"123") == XMSGS("123");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(wstring);
done = (xmsg() << std::wstring(L"123")) == XMSGS("123");
SHOW_TEST_RESULT;

#ifndef XLIB_NOCXX20
SHOW_TEST_HEAD(char8_t);
done = (xmsg() << char8_t(u8'1')) == XMSGS("1");
SHOW_TEST_RESULT;
#endif

SHOW_TEST_HEAD(char8_t*);
done = (xmsg() << (const char8_t*)u8"123") == XMSGS("123");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(std::u8string);
done = (xmsg() << std::u8string((const char8_t*)u8"123")) == XMSGS("123");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(float);
done = (xmsg() << (float)1.0) == XMSGS("1.000000");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(double);
done = (xmsg() << (double)1.0) == XMSGS("1.000000");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xmsg);
done = (xmsg() << xmsg("123")) == XMSGS("123");
SHOW_TEST_RESULT;

SHOW_TEST_DONE;

#undef XMSGT
#undef XMSGAS
#undef XMSGWS
#undef XMSGU8