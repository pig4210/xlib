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

SHOW_TEST_HEAD(string());
done = std::string(xlib::xxstring(u8buf)) == asbuf;
SHOW_TEST_RESULT;

const xlib::xxstring cxx(u8buf);
xlib::xxstring xx(u8buf);

SHOW_TEST_HEAD(string = cxx);
std::string s(cxx);
done = s == asbuf;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(string = xx);
const std::string ss = xx;
done = ss == asbuf;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(s = cxx);
s = cxx;
done = s == asbuf;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(s = xx);
s = xx;
done = s == asbuf;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(string&);
const std::string& rs = xx;
done = (void*)&xx == (void*)&rs;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(operator wstring);
done = std::wstring(xlib::xxstring(u8buf)) == wsbuf;
SHOW_TEST_RESULT;

xlib::xxstring x(asbuf);
SHOW_TEST_HEAD(string&&);
done = (std::string(x.move_as()) == asbuf) && x.empty();
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xmsg);
done = xlib::xxstring(xlib::xmsg() << u8buf) == xlib::xxstring(u8buf);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xmsg <<);
done = (xlib::xmsg() << xlib::xxstring(u8buf)) == u8buf;
SHOW_TEST_RESULT;


SHOW_TEST_DONE;