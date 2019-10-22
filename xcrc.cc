#include "xcrc.h"

#include "xlib_test.h"

SHOW_TEST_INIT(CRC)

const std::string data("1234567890");

SHOW_TEST_HEAD(crc16);
done = 50554 == crc16(data);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(crc32);
done = 639479525 == crc32(data);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(crc64);
done = 0xB1CB31BBB4A2B2BE == crc64(data);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(crcccitt);
done = 0xB4EC == crcccitt(data);
SHOW_TEST_RESULT;

SHOW_TEST_DONE;