#include "varint.h"

#include "xlib_test.h"

SHOW_TEST_INIT(VARINT)

SHOW_TEST_HEAD(zig unsigned);
done = 0x88 == zig((uint8_t)0x88);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(zig signed);
done = 7 == zig((int16_t)-4);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(zag unsigned);
done = 0x091A2B3C == zag((uint32_t)0x12345678);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(zag signed);
done = 0x1234567812345678 == zag((int64_t)0x1234567812345678);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(varint T signed);
constexpr varint v64((int64_t)12345678);
done = "\x9C\x85\xE3\x0B" == std::string((const char*)v64.data(), v64.size());
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(varint T unsigned);
constexpr varint v32((uint32_t)0x87654321);
done = "\xA1\x86\x95\xBB\x08" == std::string((const char*)v32.data(), v32.size());
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(varint T* signed);
done = 12345678 == varint<int32_t>("\x9C\x85\xE3\x0B");
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(varint T* unsigned);
done = 0x87654321 == varint<uint32_t>("\xA1\x86\x95\xBB\x08");
SHOW_TEST_RESULT;

SHOW_TEST_DONE;