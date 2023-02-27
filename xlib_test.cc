#include "xlib_test.h"

static int g_exception_count = 0;

bool xlib_test(xlib_test_routine routine) {
  try {
    routine();
    return true;
  } catch(...) {
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