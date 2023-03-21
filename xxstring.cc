#include "xxstring.h"

#include "xlib_test.h"

SHOW_TEST_INIT(xxstring)

const std::string   asbuf((const char*)u8"AA转换测试BB");
const std::wstring  wsbuf(L"AA转换测试BB");
const std::u8string u8buf((const char8_t*)u8"AA转换测试BB");

SHOW_TEST_HEAD(string);
done = xlib::xxstring(asbuf) == xlib::xxstring(u8buf);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(wstring);
done = xlib::xxstring(wsbuf) == xlib::xxstring(u8buf);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(operator string);
done = std::string(xlib::xxstring(u8buf)) == asbuf;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(operator wstring);
done = std::wstring(xlib::xxstring(u8buf)) == wsbuf;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xmsg);
done = xlib::xxstring(xlib::xmsg() << u8buf) == xlib::xxstring(u8buf);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xmsg <<);
done = xlib::xmsg() << xlib::xxstring(u8buf) == u8buf;
SHOW_TEST_RESULT;

SHOW_TEST_DONE;