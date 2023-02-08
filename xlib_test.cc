#include "xlib_test.h"

static int g_exception_count = 0;

bool xlib_test(xlib_test_routine routine)
  {
  try
    {
    routine();
    return true;
    }
  catch(...)
    {
    ++g_exception_count;
    std::cerr << std::endl << "======== exception !!!" << std::endl;
    return false;
    }
  }

#ifdef _WIN32
#include <tchar.h>
int _tmain(int , _TCHAR*)
#else
int main()
#endif
  {
  std::cout << std::endl << "xlib test done." << std::endl;
  return g_exception_count;
  }









#include "xlog.h"
class xxlog : public xlog {
 public:
  /*
    注意到：如果让 xlog 析构时 do_out ，将失去调用重载 raw_out 的机会。
           因彼时，xxlog 部分已完成析构。
  */
  virtual ~xxlog() { do_out(); }
  virtual void raw_out(const xmsg& msg) { std::wcout << msg.tows() << std::endl; }
};
#define xslog xxlog()

#include "xsig.h"

static const auto gkb = []{xsig::dbglog = true; return true;}();

xsig operator"" _sig(const char* signature, std::size_t) {
  xsig s;
  s.make_lexs(signature);
  return s;
}

static xsig sapd = ". .{ 88 } <^A ff<ff> > :X 'ddd' AA BB"_sig;