#include "xblk.h"

#include "xlib_test.h"

SHOW_TEST_INIT(XBLK)

void* const a = (void*)0x1000;
void* const b = (void*)0x1100;
const size_t size = 0x100;
const xblk blk(b, a);

SHOW_TEST_HEAD(xblk);
done = (blk.start == a) && (blk.end == b) && (blk.size == size);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xblk NoIn);
done = blk.checkin((void*)0x1111) == xblk::NoIn;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xblk WholeIn);
done = blk.checkin((void*)0x1005) == xblk::WholeIn;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xblk HeadIn);
done = blk.checkin((void*)0x10FF, 3) == xblk::HeadIn;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xblk TailIn);
done = blk.checkin((void*)0x0FFF, 3) == xblk::TailIn;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xblk SubIn);
done = blk.checkin((void*)0x0FFF, 0x1101) == xblk::SubIn;
SHOW_TEST_RESULT;

SHOW_TEST_DONE;
