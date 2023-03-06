#include "xvarint.h"

#include "xlib_test.h"

enum xvarint_enum {
  XVE_0,
  XVE_1,
  XVE_2,
  XVE_3,
  XVE_4,
};

SHOW_TEST_INIT(xvarint)

SHOW_TEST_HEAD(zig unsigned);
done = 0x88 == xlib::xzig((uint8_t)0x88);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(zig signed);
done = 7 == xlib::xzig((int16_t)-4);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(zag unsigned);
done = 0x12345678 == xlib::xzag(xlib::xzig((uint32_t)0x12345678));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(zag signed);
done = 0x1234567812345678 == xlib::xzag(xlib::xzig((int64_t)0x1234567812345678));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xvarint T signed);
constexpr xlib::xvarint v64((int64_t)12345678);
done = "\x9C\x85\xE3\x0B" == std::string((const char*)v64.data(), v64.size());
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xvarint T unsigned);
constexpr xlib::xvarint v32((uint32_t)0x87654321);
done = "\xA1\x86\x95\xBB\x08" == std::string((const char*)v32.data(), v32.size());
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xvarint T* signed);
done = 12345678 == xlib::xvarint<int32_t>("\x9C\x85\xE3\x0B");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xvarint T* unsigned);
done = 0x87654321 == xlib::xvarint<uint32_t>("\xA1\x86\x95\xBB\x08");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xvarint enum);
auto ee = XVE_4;
auto xx = xlib::xvarint(ee);
xvarint_enum ex = xlib::xvarint<xvarint_enum>(xx.data());
done = ex == ee;
SHOW_TEST_RESULT;

SHOW_TEST_DONE;