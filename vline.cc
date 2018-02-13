#include "vline.h"

#ifdef _XLIB_TEST_

#include "ws_s.h"

ADD_XLIB_TEST(VLINE)
  {
  SHOW_TEST_INIT;

  auto done = false;
  vline tline;
  vline aa;

  SHOW_TEST_HEAD("<< void*");
  tline.clear();
  tline << (void*)12345678;
  done = (tline.size() == 4) &&
    (*(uint32*)tline.c_str() == (uint32)0x05F1C2CE);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("<< T*");
  tline.clear();
  tline << "123456";
  done = (0 == memcmp(tline.c_str(), "123456", 6));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("<< xline");
  tline.clear();
  aa.clear();
  aa << "123456";
  tline << aa;
  done = (tline.size() == 6 + 1) &&
    (*(uint8*)tline.c_str() == 0x06);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("<< basic_string");
  tline.clear();
  tline << string("12345678");
  done = (tline.size() == 8);
  SHOW_TEST_RESULT(done);
  
  SHOW_TEST_HEAD("<< T&");
  tline.clear();
  tline << (int32)12345678;
  done = (tline.size() == 4) &&
    (*(uint32*)tline.c_str() == 0x0BE3859C);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("mkhead");
  tline.clear();
  tline << "123456";
  tline.mkhead();
  done = (tline.size() == 6 + 1) &&
         (*(uint8*)tline.c_str() == 0x06);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD(">> void*");
  tline.clear();
  tline << (void*)0x12345678 << "12345678" << '\0';
  void* va;
  tline >> va;
  done = (va == (void*)0x12345678);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD(">> T*");
  char abuf[0x10];
  tline >> (char*)abuf >> cnull;
  done = (0 == memcmp(abuf, "12345678", 8));
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD(">> xline");
  aa.clear();
  aa << "123456";
  tline.clear();
  tline << aa;
  aa.clear();
  tline >> aa;
  done = (aa.size() == 6) && tline.empty();
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD(">> basic_string&");
  string bs;
  tline.clear();
  tline << "123456";
  tline >> bs;
  done == (bs.size() == 6) && tline.empty();
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD(">> T&");
  tline.clear();
  tline << (int32)0x12345678;
  int32 ai;
  tline >> ai;
  done = (0x12345678 == ai);
  SHOW_TEST_RESULT(done);

  
  SHOW_TEST_HEAD(">> T const&");
  tline.clear();
  tline << cnull;
  tline >> cnull;
  done = tline.empty();
  SHOW_TEST_RESULT(done);
  }

#endif  // _XLIB_TEST_