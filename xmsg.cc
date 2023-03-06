#include "xmsg.h"

#include "xlib_test.h"

#define XMSGS(text) std::u8string((const char8_t*)u8 ## text)
#define XMSGWS(v) xlib::ws2u8(v)

SHOW_TEST_INIT(xmsg)

SHOW_TEST_HEAD(constructor as);
done = xlib::xmsg(std::string("AA\xD7\xAA\xBB\xBB\xB2\xE2\xCA\xD4\x42\x42")) == XMSGWS(L"AA转换测试BB");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(constructor ws);
done = xlib::xmsg(L"AA转换测试BB") == XMSGWS(L"AA转换测试BB");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(constructor u8);
done = xlib::xmsg(XMSGS("AA转换测试BB")) == XMSGWS(L"AA转换测试BB");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(prt);
done = xlib::xmsg().prt("%s", "AA\xD7\xAA\xBB\xBB\xB2\xE2\xCA\xD4\x42\x42") == XMSGWS(L"AA转换测试BB");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(int8_t);
done = (xlib::xmsg() << (int8_t)-1) == XMSGS("-1");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(uint8_t);
done = (xlib::xmsg() << (uint8_t)1) == XMSGS("01");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(int16_t);
done = (xlib::xmsg() << (int16_t)-1) == XMSGS("-1");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(uint16_t);
done = (xlib::xmsg() << (uint16_t)255) == XMSGS("00FF");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(int32_t);
done = (xlib::xmsg() << (int32_t)-2) == XMSGS("-2");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(uint32_t);
done = (xlib::xmsg() << (uint32_t)4294967294) == XMSGS("FFFFFFFE");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(int64_t);
done = (xlib::xmsg() << (int64_t)0x112210F47DE98115) == XMSGS("1234567890123456789");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(uint64_t);
done = (xlib::xmsg() << (uint64_t)1234567890123456789) == XMSGS("112210F47DE98115");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(void*);
#if defined(_WIN64) || defined(__amd64)
  done = (xlib::xmsg() << (void*)0x12345678) == XMSGS("0000000012345678");
#else
  done = (xlib::xmsg() << (void*)0x12345678) == XMSGS("12345678");
#endif
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(bool);
done = (xlib::xmsg() << true) == XMSGS("true");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(char);
done = (xlib::xmsg() << '1') == XMSGS("1");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(char*);
done = (xlib::xmsg() << "123") == XMSGS("123");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(string);
done = (xlib::xmsg() << std::string("123")) == XMSGS("123");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(wchar_t);
done = (xlib::xmsg() << L'1') == XMSGS("1");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(wchar_t*);
done = (xlib::xmsg() << L"123") == XMSGS("123");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(wstring);
done = (xlib::xmsg() << std::wstring(L"123")) == XMSGS("123");
SHOW_TEST_RESULT;

#ifdef __cpp_char8_t
SHOW_TEST_HEAD(char8_t);
done = (xlib::xmsg() << char8_t(u8'1')) == XMSGS("1");
SHOW_TEST_RESULT;
#endif

SHOW_TEST_HEAD(char8_t*);
done = (xlib::xmsg() << (const char8_t*)u8"123") == XMSGS("123");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(std::u8string);
done = (xlib::xmsg() << std::u8string((const char8_t*)u8"123")) == XMSGS("123");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(float);
done = (xlib::xmsg() << (float)1.0) == XMSGS("1.000000");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(double);
done = (xlib::xmsg() << (double)1.0) == XMSGS("1.000000");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xmsg);
done = (xlib::xmsg() << xlib::xmsg("123")) == XMSGS("123");
SHOW_TEST_RESULT;

SHOW_TEST_DONE;

#undef XMSGS
#undef XMSGWS