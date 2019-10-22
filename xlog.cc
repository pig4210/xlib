#define XLOGOUT(msg) (msg);
#include "xlog.h"

#include "xlib_test.h"

SHOW_TEST_INIT(XLOG)

SHOW_TEST_HEAD(xlog);
done = (xlog() << "123") == std::string("123");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xlog ex);
done = (xlog() << "123" << xlog::out << "1234") == std::string("1234");
SHOW_TEST_RESULT;

SHOW_TEST_DONE;