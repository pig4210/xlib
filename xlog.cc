#include "xlog.h"

#undef xlog_static_lvl
#define xlog_static_lvl xlib::xlog::warn

#include "xlib_test.h"

class xlog_ex : public xlib::xmsg, public xlib::xlog_out {
 public:
  ~xlog_ex() { do_out(*this); }
  virtual void raw_out(const xlib::xmsg& msg) {
    check = msg;
  }
  static xlib::xmsg check;
};
xlib::xmsg xlog_ex::check;

class xxlog : public xlib::xmsg, public xlib::xlog_out {
 public:
  ~xxlog() { do_out(*this, 200); }
  virtual void raw_out(const xlib::xmsg& msg) {
    std::wcout << msg.tows() << std::endl;
  }
};

#undef xlog_do
#define xlog_do(v) if constexpr ((v) <= xlog_static_lvl) xxlog()

SHOW_TEST_INIT(xlog)

SHOW_TEST_HEAD(xlog);
xlog_ex() << XTEXT("xlog ex");
done = xlog_ex::check == xlib::xmsg(XTEXT("xlog ex"));
SHOW_TEST_RESULT;

xxlog() << XTEXT("xlog msg 200");

xtrace << L"xlog trace xxxxxxxxx";
xfail << "xlog fail ok";

SHOW_TEST_DONE;