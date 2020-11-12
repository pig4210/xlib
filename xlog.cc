#define XLOGOUT(msg) (nullptr == msg);
#define XLOG_MAX_BYTES 200
#include "xlog.h"

#undef xlog_static_lvl
#define xlog_static_lvl xlog::warn

#undef XLOG_MAX_BYTES

#include "xlib_test.h"

SHOW_TEST_INIT(XLOG)

SHOW_TEST_HEAD(xlog);
done = (xlog() << L"123") == xmsg_base_ws(L"123");
SHOW_TEST_RESULT;

xtrace << L"xlog trace";
xfail << L"xlog fail";

SHOW_TEST_DONE;