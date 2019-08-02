/**
  \file  xrand.h
  \brief 定义了随机数的生成函数（随机平均率无法保证）。

  \version    1.2.0.161114
  \note       For All

  \author     triones
  \date       2013-01-08

  \details    Windows 使用 rdtsc 作为随机种子， Linux 使用 CLOCK_REALTIME 作为随机种子。
  
  \section history 版本记录
  
  - 2013-01-11 新建 xrand 函数 。 0.1
  - 2013-11-29 改进 xrand 函数以适应 x64 。 1.0 。
  - 2016-07-19 优化 xrand 。 1.1 。
  - 2016-11-14 适配 Linux g++ 。 1.2 。
*/
#ifndef _XLIB_XRAND_H_
#define _XLIB_XRAND_H_

#include "xlib_base.h"

/**
  用于生成随机数
  \param in mod   指定取模。默认为 0 ，不取模。
  \return         返回随机数。

  \code
    auto randvalue = xrand();
  \endcode
*/
size_t xrand(const size_t mod = 0);

#endif  // _XLIB_XRAND_H_