#include "swap.h"

#ifdef _XLIB_TEST_

ADD_XLIB_TEST(SWAP)
  {
  SHOW_TEST_INIT;

  auto done = false;

  SHOW_TEST_HEAD("bswap char");
  done = (bswap((int8)0x12) == 0x12);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("bswap short");
  done = (bswap((int16)0x1234) == 0x3412);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("bswap int");
  done = (bswap((int32)0x12345678) == 0x78563412);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("bswap int64");
  done = (bswap((int64)0x1234567812345678) == 0x7856341278563412);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("seq bswap");
  void* a = (void*)0x2;
  void* b = (void*)0x1;
  done = seqswap(a, b);
  SHOW_TEST_RESULT(done);
  }

#endif  // _XLIB_TEST_