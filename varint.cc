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


SHOW_TEST_HEAD(tovarint signed);
done = "\x9C\x85\xE3\x0B" == tovarint((int32_t)12345678);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(tovarint unsigned);
done = "\xCE\xC2\xF1\x05" == tovarint((uint64_t)12345678);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(getvarint signed);
int32_t iv;
done = 4 == getvarint(iv, "\x9C\x85\xE3\x0B", 4 ) &&
      12345678 == iv;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(getvarint unsigned);
uint32_t uv;
done = 4 == getvarint(uv, "\xCE\xC2\xF1\x05", 4) &&
       12345678 == uv;
SHOW_TEST_RESULT;

SHOW_TEST_DONE;