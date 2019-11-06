#define XLOGOUT(msg) (nullptr == msg);
#define XLOG_MAX_BYTES 200
#include "xlog.h"

#undef xlog_static_lvl
#define xlog_static_lvl xlog::warn

#undef XLOG_MAX_BYTES

#include "xlib_test.h"

SHOW_TEST_INIT(XLOG)

SHOW_TEST_HEAD(xlog);
done = (xlog() << "123") == std::string("123");
SHOW_TEST_RESULT;

xtrace << "xlog trace";
xfail << "xlog fail";

SHOW_TEST_DONE;