#include "xlog.h"

xlog::levels xlog::dynamic_lvl = xlog::lvl_on;

size_t xlog::dynamic_type = (size_t)(0 - 1);

size_t xlog::max_bytes = 0;

xlog::~xlog()
  {
  out();
  }

#if defined(_XLIB_TEST_) || !defined(_WIN32)

#include <iostream>
using std::cout;
using std::endl;

#endif

static void outs(const std::string& msg)
  {
#if !defined(_XLIB_TEST_) && defined(_WIN32)
#   ifndef  FOR_RING0
  OutputDebugStringA(msg.c_str());
#   else
  DbgPrint("%s\n",msg.c_str());
#   endif
#else
  cout << msg << endl;
#endif
  }

void xlog::out()
  {
  if(empty())  return;
  if(max_bytes == 0)
    {
    outs(*this);
    }
  else
    {
    xmsg msg;
    for(auto it = begin(); it < end(); it += max_bytes)
      {
      const auto xit = (it + max_bytes > end()) ? end() : it + max_bytes;
      msg.assign(it, xit);
      outs(msg);
      }
    }
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

size_t xlog::set_max(const size_t max)
  {
  const auto old = max_bytes;
  max_bytes = max;
  return old;
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