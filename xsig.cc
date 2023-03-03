#include "xlib_test.h"
#include "xlog.h"

class xxlog : public xlib::xlog {
 public:
  virtual ~xxlog() {
    do_out();
  }
  virtual void raw_out(const xlib::xmsg& msg) {
    std::cout << msg.toas() << std::endl;
  }
};

#define xslog xxlog()
#define xsig_need_debug
#include "xsig.h"


xlib::xsig operator"" _sig(const char* signature, std::size_t) {
  xlib::xsig s;
  s.make_lexs(signature);
  return s;
}

SHOW_TEST_INIT(XSIG)

xlib::xsig::dbglog = false;

const std::string ss(xlib::hex2bin(std::string(
    "FF50C745E800000000E80000C745FC00000000E8"
    )));

auto sig = "0000C745FC00000000E8"_sig;

SHOW_TEST_HEAD(xsig);

done = sig.match({xlib::xblk(ss.data(), ss.size())});

if (done) {
  const auto rep = sig.report(nullptr);
  done = rep.begin()->second.p == (ss.data() + 10);
}

SHOW_TEST_RESULT;

SHOW_TEST_HEAD(xsig bin);

sig = "11223344..7788<W WWW>00"_sig;
auto bs = sig.to_bin();
xlib::xsig sigx;
sigx.from_bin(bs);

done = bs.empty();

SHOW_TEST_RESULT;

SHOW_TEST_DONE;