#include "xswap.h"

#include "xlib_test.h"

SHOW_TEST_INIT(SWAP)

SHOW_TEST_HEAD(bswap int8_t);
done = (bswap((int8_t)0x12) == 0x12);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(bswap int16_t);
done = (bswap((int16_t)0x1234) == 0x3412);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(bswap int32_t);
done = (bswap((int32_t)0x12345678) == 0x78563412);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(bswap int64_t);
done = (bswap((int64_t)0x1234567812345678) == 0x7856341278563412);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(seq bswap);
void* a = (void*)0x2;
void* b = (void*)0x1;
done = seqswap(a, b);
SHOW_TEST_RESULT;

SHOW_TEST_DONE;