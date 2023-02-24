#include "xlog.h"

#undef xlog_static_lvl
#define xlog_static_lvl xlib::xlog::warn

#undef XLOG_MAX_BYTES

#include "xlib_test.h"

class xxlog : public xlib::xlog {
 public:
  virtual ~xxlog() {
    // 注意到：如果让 xlog 析构时 do_out ，将失去调用重载 raw_out 的机会。
    //        因彼时，xxlog 部分已完成析构。
    do_out();
  }
  virtual void raw_out(const xlib::xmsg& msg) {
    std::wcout << msg.tows() << std::endl;
  }
};

#undef xlog_do
#define xlog_do(v) if constexpr ((v) <= xlog_static_lvl) xxlog()

SHOW_TEST_INIT(XLOG)

xxlog() << u8"xlog msg";

SHOW_TEST_HEAD(xlog);
done = true;
SHOW_TEST_RESULT;

xtrace << L"xlog trace xxxxxxxxx";
xfail << "xlog fail ok";

SHOW_TEST_DONE;