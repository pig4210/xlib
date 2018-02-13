#include "xblk.h"

#include "swap.h"

xblk::xblk()
:_start(nullptr), _end(nullptr), _size(0)
  {

  }

xblk::xblk(void* starts, void* ends)
:_start(starts),_end(ends)
  {
  seqswap(_start, _end);
  _size = (size_t)_end - (size_t)_start;
  }

xblk::xblk(void* starts, const intptr_t sizes)
:_start(starts),_size(sizes)
  {
  _end = (void*)((intptr_t)starts + sizes);
  seqswap(_start, _end);
  _size = (size_t)_end - (size_t)_start;
  }

void* xblk::start() const
  {
  return _start;
  }

void* xblk::end() const
  {
  return _end;
  }

size_t xblk::size() const
  {
  return _size;
  }

xblk::PosDcrpt xblk::checkin(void* starts, void* ends) const
  {
  const uint8* const s = (const uint8*)start();
  const uint8* const e = (const uint8*)end();
  const uint8* ss = (const uint8*)starts;
  const uint8* ee = (const uint8*)ends;
  seqswap(ss, ee);
  if(ss < s)
    {
    if(ee < s)  return PD_NoIn;
    if(ee > e)  return PD_SubIn;
    return PD_TailIn;
    }
  if(ss > e)  return PD_NoIn;
  if(ee > e)  return PD_HeadIn;
  return PD_WholeIn;
  }

xblk::PosDcrpt xblk::checkin(void* starts, const intptr_t sizes) const
  {
  return checkin(starts, (void*)((intptr_t)starts + sizes));
  }

xblk::PosDcrpt xblk::checkin(const xblk &b) const
  {
  return checkin(b.start(), b.end());
  }

bool xblk::operator==(const xblk& b) const
  {
  if(start() != b.start()) return false;
  if(end() != b.end())  return false;
  return true;
  }

bool xblk::operator!=(const xblk& b) const
  {
  return !this->operator==(b);
  }

#ifdef _XLIB_TEST_

ADD_XLIB_TEST(XBLK)
  {
  SHOW_TEST_INIT;

  auto done = false;

  void* s = (void*)0x1000;
  void* e = (void*)0x1100;
  const size_t size = 0x100;
  xblk blk(e, s);

  SHOW_TEST_HEAD("xblk");
  done = (blk.start() == s) && (blk.end() == e) && (blk.size() == size);
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("xblk checkin PD_NoIn");
  done = blk.checkin((void*)0x1111) == xblk::PD_NoIn;
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("xblk checkin PD_WholeIn");
  done = blk.checkin((void*)0x1005) == xblk::PD_WholeIn;
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("xblk checkin PD_HeadIn");
  done = blk.checkin((void*)0x10FF, 3) == xblk::PD_HeadIn;
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("xblk checkin PD_TailIn");
  done = blk.checkin((void*)0x0FFF, 3) == xblk::PD_TailIn;
  SHOW_TEST_RESULT(done);

  SHOW_TEST_HEAD("xblk checkin PD_SubIn");
  done = blk.checkin((void*)0x0FFF, 0x1101) == xblk::PD_SubIn;
  SHOW_TEST_RESULT(done);
  }

#endif  // _XLIB_TEST_