#include "xrand.h"

#include "xlib_test.h"

#ifndef __cpp_lib_bitops
#pragma message("xlib without <bit>")
#endif

SHOW_TEST_INIT(xrand)

SHOW_TEST_HEAD(xrand);
done = xlib::xrand(0x10000) < 0x10000;
SHOW_TEST_RESULT;

std::cout << xlib::xrand() << std::endl;
std::cout << xlib::xrand() << std::endl;
std::cout << xlib::xrand() << std::endl;
std::cout << xlib::xrand() << std::endl;

SHOW_TEST_DONE;