#include "xlib_test.h"

#ifdef _XLIB_TEST_

#include <map>

/// 做成函数即保证初始化，也保证内部访问
static std::map<string, xlib_test_routine>& routines()
  {
  static std::map<string, xlib_test_routine> xlib_test_routines;
  return xlib_test_routines;
  }

bool Add_XLIB_TEST_ROUTINE(const char* const name, xlib_test_routine func)
  {
  routines()[name] = func;
  return name != nullptr;
  }

#ifdef _WIN32
#include <tchar.h>
int _tmain(int , _TCHAR* )
#else
int main()
#endif
  {
  int ret = 0;
  for(const auto& r : routines())
    {
    try
      {
      r.second();
      }
    catch(...)
      {
      ++ret;
      std::cerr << endl << "======== exception !!!" << endl;
      }
    }
  cout << endl << "xlib test done." << endl;
  return ret;
  }

#endif  // _XLIB_TEST_