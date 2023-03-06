#include "xcodecvt.h"

#include "xlib_test.h"

#ifndef __cpp_char8_t
#pragma message("xlib define char8_t")
#endif

#ifndef __cpp_lib_char8_t
#pragma message("xlib define std::u8string")
#endif

#if defined(_WIN32) && _MSVC_LANG <= 202002L
#pragma message("xlib use xcodecvt_win.h")
#endif

SHOW_TEST_INIT(xcodecvt)

// 测试字符串："AA转换测试BB\0CC" 。 g++ 默认编码 UTF8 ，为了通用， ASCII 书写使用硬编码。
// 特意加上 0 编码用于完整转换测试。
const std::string   asbuf("AA\xD7\xAA\xBB\xBB\xB2\xE2\xCA\xD4\x42\x42\0CC", 15);
const std::wstring  wsbuf(L"AA转换测试BB\0CC", 11);
const std::u8string u8buf((const char8_t*)u8"AA转换测试BB\0CC", 19);

SHOW_TEST_HEAD(as2ws);
done = wsbuf == xlib::as2ws(asbuf);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(ws2as);
done = asbuf == xlib::ws2as(wsbuf);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(u82ws);
done = wsbuf == xlib::u82ws(u8buf);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(ws2u8);
done = u8buf == xlib::ws2u8(wsbuf);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(as2u8);
done = u8buf == xlib::as2u8(asbuf);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(u82as);
done = asbuf == xlib::u82as(u8buf);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(u82ws emoji);
done = L"🚚" == xlib::u82ws((const char8_t*)u8"🚚");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(ws2u8 emoji);
done = (const char8_t*)u8"🚚" == xlib::ws2u8(L"🚚");
SHOW_TEST_RESULT;

SHOW_TEST_DONE;