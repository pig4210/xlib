/**
  \file  xrand.h
  \brief 定义了随机数的生成模板。（随机平均率无法保证、刻意忽略非线程安全。）

  \version    2.0.0.190920
  \note       For All

  \author     triones
  \date       2013-01-08

  \section history 版本记录
  
  - 2013-01-11 新建 xrand 函数 。 0.1
  - 2013-11-29 改进 xrand 函数以适应 x64 。 1.0 。
  - 2016-07-19 优化 xrand 。 1.1 。
  - 2016-11-14 适配 Linux g++ 。 1.2 。
  - 2019-09-20 重构 xrand 。允许 x86 下获取 64 bit 随机值。 2.0 。
*/
#ifndef _XLIB_XRAND_H_
#define _XLIB_XRAND_H_

#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

#include <climits>
#include <cstdint>

/// 简单单例 XRAND 对象，共用唯一的 seed 。
class XRAND
  {
  private:
    XRAND():seed(__rdtsc()) {}
    XRAND(const XRAND&) = delete;
    XRAND& operator=(const XRAND&) = delete;
  public:
    static XRAND& instance()
      {
      static XRAND obj;
      return obj;
      }
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable:4244)
#endif
    template<typename T> T operator()(T const& mod)
      {
      const auto r = __rdtsc();
      const int l = r % (CHAR_BIT * sizeof(size_t));
      seed += (r << 32) + _lrotr(r, l);
      return (0 != mod) ? (seed % mod) : (seed);
      }
#ifdef _WIN32
#pragma warning(pop)
#endif
    /// 允许 `T v = xrand();` 的写法。
    template<typename T> T operator()()
      {
      return operator()<T>(0);
      }
    /// 允许 `auto v = xrand();` 的写法。
    size_t operator()()
      {
      return operator()<size_t>(0);
      }
    /// 允许 `T v = xrand;` 的写法。
    template<typename T> operator T()
      {
      return operator()<T>();
      }
  private:
    uint64_t seed;
  };
/**
  用于生成随机数
  \param in mod   指定取模。默认为 0 ，不取模。
  \return         返回随机数。

  \code
    auto randvalue = xrand();
  \endcode
*/
#define xrand (XRAND::instance())

#endif  // _XLIB_XRAND_H_