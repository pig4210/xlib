#include "xrand.h"

#include "xlib_test.h"

SHOW_TEST_INIT(XRAND)

SHOW_TEST_HEAD(xrand);
done = xlib::xrand(0x10000) < 0x10000;
SHOW_TEST_RESULT;

std::cout << xlib::xrand() << std::endl;
std::cout << xlib::xrand() << std::endl;
std::cout << xlib::xrand() << std::endl;
std::cout << xlib::xrand() << std::endl;

SHOW_TEST_DONE;