#include "xmsg.h"

#include "xlib_test.h"

SHOW_TEST_INIT(XMSG)

SHOW_TEST_HEAD(string constructor as);
done = (xmsg("123") == std::string("123"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(string constructor utf8);
done = (xmsg(u8"AA转换测试BB", true) == std::string("AA\xD7\xAA\xBB\xBB\xB2\xE2\xCA\xD4\x42\x42"));
SHOW_TEST_RESULT;

#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4310)  // 类型强制转换截断常量值。
#endif
SHOW_TEST_HEAD(int8_t);
done = ((xmsg() << (int8_t)255) == std::string("-1"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(uint8_t);
done = ((xmsg() << (uint8_t)1) == std::string("01"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(int16_t);
done = ((xmsg() << (int8_t)65535) == std::string("-1"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(uint16_t);
done = ((xmsg() << (uint16_t)255) == std::string("00FF"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(int32_t);
done = ((xmsg() << (int32_t)4294967294) == std::string("-2"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(uint32_t);
done = ((xmsg() << (uint32_t)4294967294) == std::string("FFFFFFFE"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(int64_t);
done = ((xmsg() << (int64_t)0x112210F47DE98115) == std::string("1234567890123456789"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(uint64_t);
done = ((xmsg() << (uint64_t)1234567890123456789) == std::string("112210F47DE98115"));
SHOW_TEST_RESULT;
#ifdef _WIN32
#pragma warning(pop)
#endif

SHOW_TEST_HEAD(void*);
#if defined(_WIN64) || defined(__amd64)
  done = ((xmsg() << (void*)0x12345678) == std::string("0000000012345678"));
#else
  done = ((xmsg() << (void*)0x12345678) == std::string("12345678"));
#endif
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(char);
done = ((xmsg() << '1') == std::string("1"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(char*);
done = ((xmsg() << "123") == std::string("123"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(bool);
done = ((xmsg() << true) == std::string(":true"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(wchar_t);
done = ((xmsg() << L'1') == std::string("1"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(wchar_t*);
done = ((xmsg() << L"123") == std::string("123"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(wstring);
done = ((xmsg() << std::wstring(L"123")) == std::string("123"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(float);
done = ((xmsg() << (float)1.0) == std::string("1.000000"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(double);
done = ((xmsg() << (double)1.0) == std::string("1.000000"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(string);
done = ((xmsg() << std::string("123")) == std::string("123"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xmsg);
done = ((xmsg() << xmsg("123")) == std::string("123"));
SHOW_TEST_RESULT;

SHOW_TEST_DONE;