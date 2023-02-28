#define xsig_need_debug
#include "xsig.h"

#include "xlib_test.h"

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

SHOW_TEST_DONE;