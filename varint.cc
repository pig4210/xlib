#include "varint.h"
using std::string;

#ifdef _XLIB_TEST_

ADD_XLIB_TEST(VARINT)
  {
  SHOW_TEST_INIT;

  auto done = false;

  const string data("12345678");

  SHOW_TEST_HEAD("tovarint signed");
  done = (tovarint((int32)12345678) == "\x9C\x85\xE3\x0B");
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("tovarint unsigned");
  done = (tovarint((uint64)12345678) == "\xCE\xC2\xF1\x05");
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("getvarint signed");
  int32 iv;
  size_t len;
  len = getvarint(iv, "\x9C\x85\xE3\x0B", 4 );
  done = ((len == 4) && (iv == 12345678));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("getvarint unsigned");
  uint32 uv;
  len = getvarint(uv, "\xCE\xC2\xF1\x05", 4);
  done = ((len == 4) && (uv == 12345678));
  SHOW_TEST_RESULT(done);
  }

#endif  // _XLIB_TEST_