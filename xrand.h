﻿/**
  \file  xrand.h
  \brief 定义了随机数的生成模板。（随机平均率无法保证、刻意忽略非线程安全。）

  \version    3.0.1.210620

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
#include <bit>

/**
  用于生成随机数
  \param in mod   指定取模。默认为 0 ，不取模。
  \return         返回随机数。

  \code
    auto randvalue = xrand();
  \endcode
*/
inline uint64_t xrand(const uint64_t mod = 0)
  {
  static auto seed = __rdtsc();   // 经验证 seed 全局唯一。
  const auto r = __rdtsc();
  const int l = r % (CHAR_BIT * sizeof(size_t));
  seed += std::rotr(r, l);
  return (0 != mod) ? (seed % mod) : (seed);
  }

#endif  // _XLIB_XRAND_H_