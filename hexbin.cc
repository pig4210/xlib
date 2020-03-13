#include "hexbin.h"

#include "xlib_test.h"

SHOW_TEST_INIT(HEXBIN)

SHOW_TEST_HEAD(bin2hex as);
done = "3132333435363738" == bin2hex(std::string("12345678"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(bin2hex ws);
#ifdef _WIN32
done = "3100320033003400" == bin2hex(std::wstring(L"1234"));
#else
done = "3100000032000000" == bin2hex(std::wstring(L"12"));
#endif
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(hex2value as);
done = 0x12345678 == hex2value<uint32_t>(std::string("12345678"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(hex2value ws);
done = 0x12345678 == hex2value<int64_t>(std::wstring(L"123 456MM78"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(hex2bin as);
done = "\x12\x34\x56\x78" == hex2bin(std::string("12345678"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(hex2bin ws);
done = "\x12\x34\x56\x78" == hex2bin(std::wstring(L"12345678"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(escape as);
done = "12\'\"\?\\\a\b\f\n\r\t\v\041\41K\x41-NDCBA!0A4xK" == escape(std::string(
  R"(12\'\"\?\\\a\b\f\n\r\t\v\041\41K\x41\u4E2D\U41424344\410\x414\xK)"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(escape ws);
#ifdef _WIN32
done = L"12\'\"\?\\\a\b\f\n\r\t\v!!0!KAA0AKxK文中文" == escape(std::wstring(
  LR"(12\'\"\?\\\a\b\f\n\r\t\v\041\410\41K\x0041\x00410\x0041K\xK\u6587\U65874E2D)"));
#else
done = L"12\'\"\?\\\a\b\f\n\r\t\v!!0!KAA0AKxK文\U65874E2D" == escape(std::wstring(
  LR"(12\'\"\?\\\a\b\f\n\r\t\v\041\410\41K\x00000041\x000000410\x00000041K\xK\u6587\U65874E2D)"));
#endif
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(showbin as);
const std::string as("HIJKLMNOPQRSTUV\xD7\xAA\xBB\xBB\xB2\xE2\xCA\xD4\x42\x42\0CC", 0x1C);
const auto lpas0 = bswap((size_t)as.data());
const auto lpas1 = bswap((size_t)as.data() + 0x10);
const std::wstring asshowbin = 
  as2ws(bin2hex(&lpas0, 1)) +
  L" |48 49 4A 4B|4C 4D 4E 4F|50 51 52 53|54 55 56 D7| HIJKLMNOPQRSTUV转\r\n" +
  as2ws(bin2hex(&lpas1, 1)) +
  L" |AA BB BB B2|E2 CA D4 42|42 00 43 43|           | 换测试BB.CC\r\n";
done = asshowbin == showbin(as, SBC_ANSI);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(showbin u8);
const std::u8string us(u8"HIJKLMNOPQRSTUV转换测试BB\0CC", 0x20);
const auto lpus0 = bswap((size_t)us.data());
const auto lpus1 = bswap((size_t)us.data() + 0x10);
const std::wstring usshowbin = 
  as2ws(bin2hex(&lpus0, 1)) +
  L" |48 49 4A 4B|4C 4D 4E 4F|50 51 52 53|54 55 56 E8| HIJKLMNOPQRSTUV转\r\n" +
  as2ws(bin2hex(&lpus1, 1)) +
  L" |BD AC E6 8D|A2 E6 B5 8B|E8 AF 95 42|42 00 43 43| 换测试BB.CC\r\n";
done = usshowbin == showbin(us, SBC_UTF8);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(showbin ws);
#ifdef _WIN32
const std::wstring ws0(L"1234567");
const std::wstring ws1(L"拿转换测试");
const std::string was = std::string((const char*)ws0.c_str(), ws0.size() * sizeof(wchar_t)) +
  std::string("\xFF", 1) + std::string((const char*)ws1.c_str(), ws1.size() * sizeof(wchar_t)) +
  std::string("\0", 1);
const std::wstring ws((const wchar_t*)was.c_str(), was.size() / sizeof(wchar_t));
const auto lpws0 = bswap((size_t)ws.data());
const auto lpws1 = bswap((size_t)ws.data() + 0x10);
const std::wstring wsshowbin = 
  as2ws(bin2hex(&lpws0, 1)) +
  L" |31 00 32 00|33 00 34 00|35 00 36 00|37 00 FF FF| 1234567.拿\r\n" +
  as2ws(bin2hex(&lpws1, 1)) +
  L" |62 6C 8F 62|63 4B 6D D5|8B 00      |           | 转换测试.\r\n";
#else
const std::wstring ws(L"123转换测试\0", 8);
const auto lpws0 = bswap((size_t)ws.data());
const auto lpws1 = bswap((size_t)ws.data() + 0x10);
const std::wstring wsshowbin = 
  as2ws(bin2hex(&lpws0, 1)) +
  L" |31 00 00 00|32 00 00 00|33 00 00 00|6C 8F 00 00| 123转\r\n" +
  as2ws(bin2hex(&lpws1, 1)) +
  L" |62 63 00 00|4B 6D 00 00|D5 8B 00 00|00 00 00 00| 换测试.\r\n";
#endif
done = wsshowbin == showbin(ws, SBC_UNICODE);
SHOW_TEST_RESULT;

SHOW_TEST_DONE;