/**
  \file  xlib_test.h
  \brief 定义了 xlib 校验环境。

  \version    2.0.0.190919

  \author     triones
  \date       2013-03-19

  \section history 版本记录

  - 2013-03-19 新建 xlib_test 模块，用于校验 xlib 的正确性。 1.0 。
  - 2019-09-19 重构 xlib_test 模块。 2.0 。
*/
#ifndef _XLIB_TEST_H_
#define _XLIB_TEST_H_

#include <iomanip>
#include <iostream>
#include <string>

using xlib_test_routine = int (*)(void);

bool xlib_test(xlib_test_routine);

#define SHOW_TEST_INIT(name)            \
  static const auto gkxtb = xlib_test(  \
    [] {                                \
      std::cout << std::endl << "================ test " #name << std::endl; \
      int error_count = 0;              \
      auto done = false;
#define SHOW_TEST_DONE                  \
      std::cout << std::endl;           \
      return error_count;                   \
    }                                   \
    );
#define SHOW_TEST_HEAD(head)            \
  std::cout << std::setiosflags(std::ios::left) << std::setw(41) << #head;
#define SHOW_TEST_RESULT                \
  if (!done) ++error_count;             \
  std::cout << " : " << (done ? "ok" : "fail !!!") << std::endl;

#endif  // _XLIB_TEST_H_