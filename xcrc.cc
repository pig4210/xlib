#include "xcrc.h"

#include "xlib_test.h"

SHOW_TEST_INIT(xcrc)

SHOW_TEST_HEAD(crc16);
done = 0xC57A == xlib::crc16("1234567890", 10);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(crc32);
done = 0x261DAEE5 == xlib::crc32(std::string("1234567890"));
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(crc64);
done = 0xB1CB31BBB4A2B2BE == xlib::crc64("1234567890");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(crcccitt);
done = 0x4D53 == xlib::crcccitt(std::array<char, 2>{'1', '2'});
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(overload);
done = 
    0x2A2F0E859495CAED == xlib::crc64((      void*)"1", 1) &&
    0x2A2F0E859495CAED == xlib::crc64((const void*)"1", 1) &&
    0x2A2F0E859495CAED == xlib::crc64((      char*)"1", 1) &&
    0x2A2F0E859495CAED == xlib::crc64((const char*)"1", 1) &&
    0x2A2F0E859495CAED == xlib::crc64(std::string("1")) &&
    0x2A2F0E859495CAED == xlib::crc64("1");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(constexpr);
constexpr auto c0 = xlib::crc32("1234567890");
constexpr auto c1 = xlib::crc64(L"1");
constexpr auto c2 = xlib::crc16(u8"1234567890");
done = (c0 == 0x261DAEE5) &&
#ifdef _WIN32
       (c1 == 0x7635B8617CE753D8) &&
#else
       (c1 == 0x3D1B1331AA7B3B58) &&
#endif
       (c2 == 0xC57A);
SHOW_TEST_RESULT;

SHOW_TEST_DONE;