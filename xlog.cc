#define XLOGOUT
#define XLOG_MAX_BYTES 200

#include "xlog.h"

#undef xlog_static_lvl
#define xlog_static_lvl xlog::warn

#undef XLOG_MAX_BYTES

#include "xlib_test.h"

class xxlog : public xlog
  {
  public:
    virtual ~xxlog()
      {
      // 注意到：如果让 xlog 析构时 do_out ，将失去调用重载 raw_out 的机会。
      //        因彼时，xxlog 部分已完成析构。
      do_out();
      }
    virtual void raw_out(const xmsg&)
      {
      }
  };

#undef xlog_do
#define xlog_do(v) if constexpr ((v) <= xlog_static_lvl) xxlog()

SHOW_TEST_INIT(XLOG)

SHOW_TEST_HEAD(xlog);
done = (xxlog() << L"123") == xmsg(L"123");
SHOW_TEST_RESULT;

xtrace << L"xlog trace";
xfail << L"xlog fail";

SHOW_TEST_DONE;