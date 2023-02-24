/**
  \file  xrand.h
  \brief 定义了随机数的生成模板。（随机平均率无法保证、刻意忽略非线程安全。）

  \version    3.0.2.220105

  \author     triones
  \date       2013-01-08

  \section history 版本记录

  - 2013-01-11 新建 xrand 函数 。 0.1
  - 2013-11-29 改进 xrand 函数以适应 x64 。 1.0 。
  - 2016-07-19 优化 xrand 。 1.1 。
  - 2016-11-14 适配 Linux g++ 。 1.2 。
  - 2019-09-20 重构 xrand 。允许 x86 下获取 64 bit 随机值。 2.0 。
  - 2019-09-29 再次重构 xrand 。放弃单例。 3.0 。
  - 2021-06-20 VS2019 支持 bit 库，引入改造之。
  - 2022-01-05 引入旧代码，向下兼容 c++17 。
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

#include "xcompilerspecial.h"

#ifndef XLIB_NOCXX20
#include <bit>
#endif

namespace xlib {

/**
  用于生成随机数
  \param    mod   指定取模。默认为 0 ，不取模。
  \return         返回随机数。

  \code
    auto randvalue = xrand();
  \endcode
*/
inline uint64_t xrand(const uint64_t mod = 0) {
  static auto seed = __rdtsc();  // 经验证 seed 全局唯一。
  const auto r = __rdtsc();
  const int l = r % (CHAR_BIT * sizeof(size_t));
#ifndef XLIB_NOCXX20
  seed += std::rotr(r, l);
#else
  // x86 下的 _lrotr 行为一致。
  // x64 下的 _lrotr ， windows 使用 32 bit ， linux 使用 64 bit 。
  // x64 下的 unsigned long ， windows 使用 32 bit ， linux 使用 64 bit 。
  // 对于不支持 标准库 bit 的，采用此做法。
  seed += (r << (sizeof(uint32_t) * CHAR_BIT)) + _lrotr((unsigned long)r, l);
#endif
  return (0 != mod) ? (seed % mod) : (seed);
}

}  // namespace xlib

#endif  // _XLIB_XRAND_H_