#include "xrand.h"

#include "xlib_test.h"

SHOW_TEST_INIT(XRAND)

SHOW_TEST_HEAD(xrand((uint8_t)));
done = (xrand((uint8_t)0) < 0x100);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xrand((uint16_t)));
done = (xrand((uint16_t)0) < 0x10000);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xrand((uint32_t)));
done = (xrand((uint32_t)0) < 0x100000000);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD((uint8_t)xrand);
done = ((uint8_t)xrand < 0x100);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD((uint16_t)xrand());
done = ((uint16_t)xrand() < 0x10000);
SHOW_TEST_RESULT;

std::cout << xrand() << std::endl;
std::cout << xrand() << std::endl;
std::cout << xrand() << std::endl;
std::cout << xrand() << std::endl;
std::cout << (uint64_t)xrand << std::endl;

SHOW_TEST_DONE;