#include "hexbin.h"

#include "xlib_test.h"

SHOW_TEST_INIT(HEXBIN)

SHOW_TEST_HEAD(bin2hex as);
done = (bin2hex(std::string("12345678")) == "3132333435363738");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(bin2hex ws);
#ifdef _WIN32
done = (bin2hex(std::wstring(L"1234")) == "3100320033003400");
#else
done = (bin2hex(std::wstring(L"12")) == "3100000032000000");
#endif
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(hex2value as);
done = (hex2value<uint32_t>(std::string("12345678")) == 0x12345678);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(hex2value ws);
done = (hex2value<uint64_t>(std::wstring(L"123 456MM78")) == 0x12345678);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(hex2bin as);
done = (hex2bin(std::string("12345678")) == "\x12\x34\x56\x78");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(hex2bin ws);
done = (hex2bin(std::wstring(L"12345678")) == "\x12\x34\x56\x78");
SHOW_TEST_RESULT;

/*
  const char* const buf = "12345678";
  const char* const hex = "3132333435363738";
  string rets;

  SHOW_TEST_HEAD("showbin");
  rets = showbin(string("0123456789ABCDEF"));
  rets.erase(rets.begin(), rets.begin() + sizeof(void*) * sizeof(HEX_VALUE_STRUCT));
  done = (rets == "┃30 31 32 33|34 35 36 37|38 39 41 42|43 44 45 46┃0123456789ABCDEF\r\n");
  SHOW_TEST_RESULT(done);
*/
SHOW_TEST_DONE;
