#include "xlib_test.h"

static int g_error_count = 0;

bool xlib_test(xlib_test_routine routine) {
  try {
    g_error_count += routine();
    return true;
  } catch(...) {
    ++g_error_count;
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
  return g_error_count;
  }