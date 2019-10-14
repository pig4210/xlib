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

SHOW_TEST_HEAD(escape);
done = (escape(std::string(
  R"(12\'\"\?\\\a\b\f\n\r\t\v\041\41K\x41\u4E2D\U41424344\410\x414\xK)")) ==
    "12\'\"\?\\\a\b\f\n\r\t\v\041\41K\x41-NDCBA!0A4xK");
SHOW_TEST_RESULT;

const std::string as("HIJKLMNOPQRSTUV\xD7\xAA\xBB\xBB\xB2\xE2\xCA\xD4\x42\x42\0CC", 0x1C);
const char* const lpas0 = bswap(as.data());
const char* const lpas1 = bswap(as.data() + 0x10);
const std::string asshowbin = 
  bin2hex((void*)&lpas0, sizeof(lpas0)) +
  "┃48 49 4A 4B|4C 4D 4E 4F|50 51 52 53|54 55 56 D7┃HIJKLMNOPQRSTUV转\r\n" +
  bin2hex((void*)&lpas1, sizeof(lpas1)) +
  "┃AA BB BB B2|E2 CA D4 42|42 00 43 43|           ┃换测试BB.CC\r\n";

SHOW_TEST_HEAD(showbin as);
done = (asshowbin == showbin(as, SBC_ANSI));
SHOW_TEST_RESULT;

const std::string us(u8"HIJKLMNOPQRSTUV转换测试BB\0CC", 0x20);
const char* const lpus0 = bswap(us.data());
const char* const lpus1 = bswap(us.data() + 0x10);
const std::string usshowbin = 
  bin2hex((void*)&lpus0, sizeof(lpus0)) +
  "┃48 49 4A 4B|4C 4D 4E 4F|50 51 52 53|54 55 56 E8┃HIJKLMNOPQRSTUV转\r\n" +
  bin2hex((void*)&lpus1, sizeof(lpus1)) +
  "┃BD AC E6 8D|A2 E6 B5 8B|E8 AF 95 42|42 00 43 43┃换测试BB.CC\r\n";

SHOW_TEST_HEAD(showbin u8);
done = (usshowbin == showbin(us, SBC_UTF8));
SHOW_TEST_RESULT;

#ifdef _WIN32
const std::wstring ws0(L"1234567");
const std::wstring ws1(L"拿转换测试");
const std::string was = std::string((const char*)ws0.c_str(), ws0.size() * sizeof(wchar_t)) +
  std::string("\xFF", 1) + std::string((const char*)ws1.c_str(), ws1.size() * sizeof(wchar_t)) +
  std::string("\0", 1);
const std::wstring ws((const wchar_t*)was.c_str(), was.size() / sizeof(wchar_t));
const wchar_t* const lpws0 = bswap(ws.data());
const wchar_t* const lpws1 = bswap(ws.data() + 0x8);
const std::string wsshowbin = 
  bin2hex((void*)&lpws0, sizeof(lpws0)) +
  "┃31 00 32 00|33 00 34 00|35 00 36 00|37 00 FF FF┃1234567.拿\r\n" +
  bin2hex((void*)&lpws1, sizeof(lpws1)) +
  "┃62 6C 8F 62|63 4B 6D D5|8B 00      |           ┃转换测试.\r\n";
#else
const std::wstring ws(L"123转换测试\0", 8);
const wchar_t* const lpws0 = bswap(ws.data());
const wchar_t* const lpws1 = bswap(ws.data() + 0x4);
const std::string wsshowbin = 
  bin2hex((void*)&lpws0, sizeof(lpws0)) +
  "┃31 00 00 00|32 00 00 00|33 00 00 00|6C 8F 00 00┃123转\r\n" +
  bin2hex((void*)&lpws1, sizeof(lpws1)) +
  "┃62 63 00 00|4B 6D 00 00|D5 8B 00 00|00 00 00 00┃换测试.\r\n";
#endif

SHOW_TEST_HEAD(showbin ws);
done = (wsshowbin == showbin(ws, SBC_UNICODE));
SHOW_TEST_RESULT;

SHOW_TEST_DONE;