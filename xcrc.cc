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

SHOW_TEST_DONE;