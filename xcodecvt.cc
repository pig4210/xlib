#include "xcodecvt.h"

#include "xlib_test.h"

#ifdef XLIB_NOCXX20
#pragma message("!!! use fake std::u8string !!!")
#ifndef XCODECVTHFILE
std::locale::id std::codecvt<char16_t, char, _Mbstatet>::id;
#endif
#endif

#ifdef XCODECVTHFILE
#pragma message("!!! use XCODECVTHFILE !!!")
#endif

SHOW_TEST_INIT(XCODECVT)

// 测试字符串："AA转换测试BB\0CC" 。 g++ 默认编码 UTF8 ，为了通用， ASCII 书写使用硬编码。
// 特意加上 0 编码用于完整转换测试。
const std::string   asbuf("AA\xD7\xAA\xBB\xBB\xB2\xE2\xCA\xD4\x42\x42\0CC", 15);
const std::wstring  wsbuf(L"AA转换测试BB\0CC", 11);
const std::u8string u8buf((const char8_t*)u8"AA转换测试BB\0CC", 19);

SHOW_TEST_HEAD(as2ws);
done = wsbuf == as2ws(asbuf);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(ws2as);
done = asbuf == ws2as(wsbuf);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(u82ws);
done = wsbuf == u82ws(u8buf);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(ws2u8);
done = u8buf == ws2u8(wsbuf);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(as2u8);
done = u8buf == as2u8(asbuf);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(u82as);
done = asbuf == u82as(u8buf);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(u82ws emoji);
done = L"🚚" == u82ws((const char8_t*)u8"🚚");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(ws2u8 emoji);
done = (const char8_t*)u8"🚚" == ws2u8(L"🚚");
SHOW_TEST_RESULT;

SHOW_TEST_DONE;