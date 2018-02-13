/*!
  \file  xrand.h
  \brief xrand.h定义了随机数的生成函数(随机平均率无法保证)

  - Windows使用rdtsc作为随机种子，Linux使用CLOCK_REALTIME作为随机种子

  \version    3.0.1611.1417
  \note       For All

  \author     triones
  \date       2013-01-08
*/
#ifndef _XLIB_XRAND_H_
#define _XLIB_XRAND_H_

#include "xlib_base.h"

/*!
  用于生成随机数
  \param in mod   指定取模，默认为0，不取模
  \return         返回随机数

  \code
    auto randvalue = xrand();
  \endcode
*/
size_t xrand(const size_t mod = 0);

#endif  // _XLIB_XRAND_H_