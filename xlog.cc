#include "xlog.h"

xlog::levels xlog::dynamic_lvl = xlog::lvl_on;

size_t xlog::dynamic_type = (size_t)(0 - 1);

xlog::~xlog()
  {
  out();
  }

#if defined(_XLIB_TEST_) || !defined(_WIN32)

#include <iostream>
using std::cout;
using std::endl;

#endif

void xlog::out()
  {
  if(empty())  return;
#if !defined(_XLIB_TEST_) && defined(_WIN32)
#   ifndef  FOR_RING0
  OutputDebugStringA(c_str());
#   else
  DbgPrint("%s\n",c_str());
#   endif
#else
  cout << c_str() << endl;
#endif
  clear();
  }

xlog::levels xlog::set_level(const xlog::levels new_level)
  {
  const auto lvl = dynamic_lvl;
  dynamic_lvl = new_level;
  return lvl;
  }

size_t xlog::open_type(const size_t be_set_type)
  {
  const auto tmp_type = dynamic_type;
  dynamic_type |= ((size_t)(1) << be_set_type);
  return tmp_type;
  }

size_t xlog::close_type(const size_t be_set_type)
  {
  const auto tmp_type = dynamic_type;
  dynamic_type &= ~((size_t)(1) << be_set_type);
  return tmp_type;
  }

xlog::levels xlog::level()
  {
  return dynamic_lvl;
  }

size_t xlog::type()
  {
  return dynamic_type;
  }

xmsg& xlogout(xmsg& v)
  {
  (*(xlog*)&v).out();
  return v;
  }

#ifdef _XLIB_TEST_

ADD_XLIB_TEST(XLOG)
  {
  SHOW_TEST_INIT;

  xerr << "xlog output";
  }

#endif  // _XLIB_TEST_