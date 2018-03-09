#include "xrand.h"

#ifdef _WIN32

#pragma warning(push)
#pragma warning(disable:4244) // “初始化”: 从“ULONG64”转换到“const unsigned int”，可能丢失数据

size_t xrand(const size_t mod)
  {
  //! 由于重要性不是很高，随机数的生成可以忽视多线程读写问题
  static size_t gk_xrand_seed = 0;
  const size_t r = __rdtsc();
#if _WIN64
  const int l = r % 64;
  gk_xrand_seed += _rotl64(r, l);
#else
  const int l = r % 32;
  gk_xrand_seed += _lrotl(r, l);
#endif
  return (mod) ? (gk_xrand_seed % mod) : (gk_xrand_seed);
  }

#pragma warning(pop)

#else   // _WIN32

#include <ctime>

size_t xrand(const size_t mod)
  {
  static size_t gk_xrand_seed = 0;
  timespec t;
  clock_gettime(CLOCK_REALTIME, &t);
#ifdef __amd64
  const size_t r = t.tv_nsec * 0x100000000 + t.tv_nsec;
  const int l = r % 64;
  gk_xrand_seed += (((r) << (l)) | (((r)& 0xffffffffffffffff) >> (64 - (l))));
#else
  const size_t r = t.tv_nsec;
  const int l = r % 32;
  gk_xrand_seed += (((r) << (l)) | (((r)& 0xffffffff) >> (32 - (l))));
#endif
  return (mod) ? (gk_xrand_seed % mod) : (gk_xrand_seed);
  }

#endif  // _WIN32

#ifdef _XLIB_TEST_

ADD_XLIB_TEST(XRAND)
  {
  SHOW_TEST_INIT;

  auto done = false;

  SHOW_TEST_HEAD("xrand");
  done = ((intptr_t)xrand(0x100) < 0x100);
  SHOW_TEST_RESULT(done);

  cout << (void*)xrand() << endl;
  cout << (void*)xrand() << endl;
  cout << (void*)xrand() << endl;
  cout << (void*)xrand() << endl;
  cout << (void*)xrand() << endl;
  }

#endif  // _XLIB_TEST_