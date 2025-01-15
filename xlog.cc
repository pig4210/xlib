#include "xlog.h"

#undef xlog_static_lvl
#define xlog_static_lvl xlib::xlog::warn

#include "xlib_test.h"

class xlog_ex : public xlib::xlog {
 public:
  virtual ~xlog_ex() {
    // 注意到：如果让 xlog 析构时 do_out ，将失去调用重载 raw_out 的机会。
    //        因彼时，xxlog 部分已完成析构。
    do_out();
  }
  virtual void raw_out(const xlib::xmsg& msg) {
    check = msg;
  }
  static xlib::xmsg check;
};
xlib::xmsg xlog_ex::check;

class xxlog : public xlib::xlog {
 public:
  virtual ~xxlog() { do_out(); }
  virtual void raw_out(const xlib::xmsg& msg) {
    std::wcout << msg.tows() << std::endl;
  }
};

#undef xlog_do
#define xlog_do(v) if constexpr ((v) <= xlog_static_lvl) xxlog()

SHOW_TEST_INIT(xlog)

SHOW_TEST_HEAD(xlog);
xlog_ex log_ex;
log_ex << XTEXT("xlog ex");
log_ex.do_out();
done = xlog_ex::check == xlib::xmsg(XTEXT("xlog ex"));
SHOW_TEST_RESULT;

xxlog() << XTEXT("xlog msg");

xxlog log;
log << XTEXT("xlog msg 200");
log.do_out(200);

xtrace << L"xlog trace xxxxxxxxx";
xfail << "xlog fail ok";

SHOW_TEST_DONE;