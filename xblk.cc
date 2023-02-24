#include "xblk.h"

#include "xlib_test.h"

SHOW_TEST_INIT(XBLK)

void* const a = (void*)0x1000;
void* const b = (void*)0x1100;
const size_t size = 0x100;
const xlib::xblk blk(b, a);

SHOW_TEST_HEAD(xblk);
done = blk.start == a && blk.end == b && blk.size == size;
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xblk NoIn);
done = xlib::xblk::NoIn == blk.check((void*)0x1111);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xblk WholeIn);
done = xlib::xblk::WholeIn == blk.check((void*)0x1005);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xblk HeadIn);
done = xlib::xblk::HeadIn == blk.check((void*)0x10FF, 3);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xblk TailIn);
done = xlib::xblk::TailIn == blk.check((void*)0x0FFF, 3);
SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xblk SubIn);
done = xlib::xblk::SubIn == blk.check((void*)0x0FFF, 0x1101);
SHOW_TEST_RESULT;

SHOW_TEST_DONE;